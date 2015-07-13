/*
 * Copyright (c) 2015 Nicholas Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define MPACK_INTERNAL 1

#include "mpack-node.h"

#if MPACK_NODE

#ifndef MPACK_NODE_PAGE_INITIAL_CAPACITY
#define MPACK_NODE_PAGE_INITIAL_CAPACITY 8
#endif

/*
 * Tree Functions
 */

mpack_node_t* mpack_tree_root(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return &tree->nil_node;
    return mpack_tree_node_at(tree, 0);
}

#ifdef MPACK_MALLOC
static mpack_error_t mpack_tree_grow(mpack_tree_t* tree) {

    // grow page array if needed
    if (tree->page_count == tree->page_capacity) {
        size_t new_capacity = tree->page_capacity * 2;

        #ifdef MPACK_REALLOC
        mpack_node_t** new_pages = (mpack_node_t**)MPACK_REALLOC(tree->pages, sizeof(mpack_node_t*) * new_capacity);
        #else
        mpack_node_t** new_pages = (mpack_node_t**)MPACK_MALLOC(sizeof(mpack_node_t*) * new_capacity);
        #endif
        if (new_pages == NULL)
            return mpack_error_memory;
        #ifndef MPACK_REALLOC
        mpack_memcpy(new_pages, tree->pages, tree->page_count * sizeof(mpack_node_t*));
        MPACK_FREE(tree->pages);
        #endif

        tree->pages = new_pages;
        tree->page_capacity = new_capacity;
    }

    // allocate new page
    tree->pages[tree->page_count] = (mpack_node_t*)MPACK_MALLOC(sizeof(mpack_node_t) * MPACK_NODE_PAGE_SIZE);
    if (tree->pages[tree->page_count] == NULL)
        return mpack_error_memory;
    ++tree->page_count;

    return mpack_ok;
}
#endif

void mpack_tree_init_clear(mpack_tree_t* tree) {
    mpack_memset(tree, 0, sizeof(*tree));
    tree->nil_node.tree = tree;
    tree->nil_node.tag.type = mpack_type_nil;
}

void mpack_tree_parse(mpack_tree_t* tree, const char* data, size_t length) {

    // This function is unfortunately huge and ugly, but there isn't
    // a good way to break it apart without losing performance. It's
    // well-commented to try to make up for it.

    if (length == 0) {
        mpack_tree_init_error(tree, mpack_error_io);
        return;
    }

    // Initialize the reader. The rest of this function flags errors
    // on the reader, not the tree. We pick up the reader's error
    // state at the end of parsing.
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, length);

    // We read nodes in a loop instead of recursively for maximum
    // performance. The stack holds the amount of children left to
    // read in each level of the tree.
    struct {
        size_t child;
        size_t left;
        #if MPACK_READ_TRACKING
        bool map;
        #endif
    } stack[MPACK_NODE_MAX_DEPTH];
    mpack_memset(stack, 0, sizeof(stack));

    // We keep track of the number of possible nodes left in the data. This
    // is to ensure that malicious nested data is not trying to make us
    // run out of memory by allocating too many nodes. (For example malicious
    // data that repeats 0xDE 0xFF 0xFF would otherwise cause us to run out
    // of memory. With this, the parser can only allocate as many nodes as
    // there are bytes in the data. An error will be flagged immediately
    // if and when there isn't enough data left to fully read all children
    // of all open compound types on the stack.)
    size_t possible_nodes_left = length - 1;

    // count the first node now
    tree->node_count = 1;
    size_t level = 0;
    stack[0].child = 0;
    stack[0].left = 1;

    do {
        mpack_node_t* node = mpack_tree_node_at(tree, stack[level].child);
        --stack[level].left;
        ++stack[level].child;
        node->tree = tree;

        // Read a tag, keeping track of the number of possible nodes left. (One
        // byte has already been counted for this node.)
        size_t pos = reader.pos;
        node->tag = mpack_read_tag(&reader);
        if (reader.pos - pos - 1 > possible_nodes_left) {
            mpack_reader_flag_error(&reader, mpack_error_invalid);
            break;
        }
        possible_nodes_left -= reader.pos - pos - 1;
        mpack_log("read node tag %s\n", mpack_type_to_string(mpack_node_type(node)));

        // Handle compound types
        mpack_type_t type = mpack_node_type(node);
        switch (type) {

            case mpack_type_array:
            case mpack_type_map: {

                // Make sure we have enough room in the stack
                if (level + 1 == MPACK_NODE_MAX_DEPTH) {
                    mpack_reader_flag_error(&reader, mpack_error_too_big);
                    break;
                }

                // Calculate total elements to read
                size_t total = node->tag.v.n;
                if (type == mpack_type_map) {
                    if ((uint64_t)total * 2 > (uint64_t)SIZE_MAX) {
                        mpack_reader_flag_error(&reader, mpack_error_too_big);
                        break;
                    }
                    total *= 2;
                }

                // Each node is at least one byte. Count these bytes now to make
                // sure there is enough data left.
                if (total > possible_nodes_left) {
                    mpack_reader_flag_error(&reader, mpack_error_invalid);
                    break;
                }
                possible_nodes_left -= total;

                // Setup this node's children
                node->data.children = tree->node_count;
                tree->node_count += total;

                // Make sure we have enough nodes to store the children
                #ifdef MPACK_MALLOC
                if (tree->pages) {
                    while (tree->node_count > tree->page_count * MPACK_NODE_PAGE_SIZE) {
                        mpack_error_t error = mpack_tree_grow(tree);
                        if (error != mpack_ok) {
                            mpack_reader_flag_error(&reader, mpack_error_too_big);
                            break;
                        }
                    }
                } else
                #endif
                {
                    if (tree->node_count > tree->pool_count) {
                        tree->node_count = tree->pool_count;
                        mpack_reader_flag_error(&reader, mpack_error_too_big);
                        break;
                    }
                }

                // Push this node onto the stack to read its children
                ++level;
                stack[level].child = node->data.children;
                stack[level].left = total;
                #if MPACK_READ_TRACKING
                stack[level].map = (type == mpack_type_map);
                #endif
            } break;

            case mpack_type_str:
            case mpack_type_bin:
            case mpack_type_ext:
                if (node->tag.v.l > possible_nodes_left) {
                    mpack_reader_flag_error(&reader, mpack_error_invalid);
                    break;
                }
                possible_nodes_left -= node->tag.v.l;

                node->data.bytes = mpack_read_bytes_inplace(&reader, node->tag.v.l);
                mpack_done_type(&reader, node->tag.type);
                break;

            default:
                break;
        }

        // Pop any empty compound types from the stack
        while (level != 0 && stack[level].left == 0) {
            #if MPACK_READ_TRACKING
            stack[level].map ? mpack_done_map(&reader) : mpack_done_array(&reader);
            #endif
            --level;
        }
    } while (level != 0 && mpack_reader_error(&reader) == mpack_ok);

    size_t remaining = mpack_reader_remaining(&reader, NULL);
    tree->size = length - remaining;
    tree->error = mpack_reader_destroy(&reader);

    // This seems like a bug / performance flaw in GCC. In release the
    // below assert would compile to:
    //
    //     (!(possible_nodes_left == remaining) ? __builtin_unreachable() : ((void)0))
    //
    // This produces identical assembly with GCC 5.1 on ARM64 under -O3, but
    // with -O3 -flto, node parsing is over 4% slower. This should be a no-op
    // even in -flto since the function ends here and possible_nodes_left
    // does not escape this function.
    //
    // Leaving a TODO: here to explore this further. In the meantime we preproc it
    // under MPACK_DEBUG.
    #if MPACK_DEBUG
    mpack_assert(possible_nodes_left == remaining,
            "incorrect calculation of possible nodes! %i possible nodes, but %i bytes remaining",
            (int)possible_nodes_left, (int)remaining);
    #endif
}

#ifdef MPACK_MALLOC
void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length) {
    mpack_tree_init_clear(tree);

    tree->page_count = 0;
    tree->page_capacity = MPACK_NODE_PAGE_INITIAL_CAPACITY;
    tree->pages = (mpack_node_t**)MPACK_MALLOC(sizeof(mpack_node_t*) * tree->page_capacity);
    if (tree->pages == NULL) {
        tree->error = mpack_error_memory;
        return;
    }

    tree->pages[tree->page_count] = (mpack_node_t*)MPACK_MALLOC(sizeof(mpack_node_t) * MPACK_NODE_PAGE_SIZE);
    if (tree->pages[tree->page_count] == NULL) {
        tree->error = mpack_error_memory;
        return;
    }
    ++tree->page_count;

    mpack_tree_parse(tree, data, length);
}
#endif

void mpack_tree_init_pool(mpack_tree_t* tree, const char* data, size_t length, mpack_node_t* node_pool, size_t node_pool_count) {
    mpack_tree_init_clear(tree);
    tree->pool = node_pool;
    tree->pool_count = node_pool_count;
    mpack_tree_parse(tree, data, length);
}

void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_memset(tree, 0, sizeof(*tree));

    tree->nil_node.tree = tree;
    tree->nil_node.tag.type = mpack_type_nil;

    tree->error = error;
}

#if MPACK_STDIO
typedef struct mpack_file_tree_t {
    char* data;
    size_t size;
    char buffer[MPACK_BUFFER_SIZE];
} mpack_file_tree_t;

static void mpack_file_tree_teardown(mpack_tree_t* tree) {
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*)tree->context;
    MPACK_FREE(file_tree->data);
    MPACK_FREE(file_tree);
}

static bool mpack_file_tree_read(mpack_tree_t* tree, mpack_file_tree_t* file_tree, const char* filename, size_t max_size) {

    // open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }

    // get the file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }
    if (size == 0) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_invalid);
        return false;
    }

    // make sure the size is less than max_size
    // (this mess exists to safely convert between long and size_t regardless of their widths)
    if (max_size != 0 && (((uint64_t)LONG_MAX > (uint64_t)SIZE_MAX && size > (long)SIZE_MAX) || (size_t)size > max_size)) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_too_big);
        return false;
    }

    // allocate data
    file_tree->data = (char*)MPACK_MALLOC(size);
    if (file_tree->data == NULL) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_memory);
        return false;
    }

    // read the file
    long total = 0;
    while (total < size) {
        size_t read = fread(file_tree->data + total, 1, (size_t)(size - total), file);
        if (read <= 0) {
            fclose(file);
            mpack_tree_init_error(tree, mpack_error_io);
            MPACK_FREE(file_tree->data);
            return false;
        }
        total += read;
    }

    fclose(file);
    file_tree->size = (size_t)size;
    return true;
}

void mpack_tree_init_file(mpack_tree_t* tree, const char* filename, size_t max_size) {

    // the C STDIO family of file functions use long (e.g. ftell)
    if (max_size > LONG_MAX) {
        mpack_break("max_size of %"PRIu64" is invalid, maximum is LONG_MAX", (uint64_t)max_size);
        mpack_tree_init_error(tree, mpack_error_too_big);
        return;
    }

    // allocate file tree
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*) MPACK_MALLOC(sizeof(mpack_file_tree_t));
    if (file_tree == NULL) {
        mpack_tree_init_error(tree, mpack_error_memory);
        return;
    }

    // read all data
    if (!mpack_file_tree_read(tree, file_tree, filename, max_size)) {
        MPACK_FREE(file_tree);
        return;
    }

    mpack_tree_init(tree, file_tree->data, file_tree->size);
    mpack_tree_set_context(tree, file_tree);
    mpack_tree_set_teardown(tree, mpack_file_tree_teardown);
}
#endif

mpack_error_t mpack_tree_destroy(mpack_tree_t* tree) {
    #ifdef MPACK_MALLOC
    if (tree->pages) {
        for (size_t i = 0; i < tree->page_count; ++i)
            MPACK_FREE(tree->pages[i]);
        MPACK_FREE(tree->pages);
        tree->pages = NULL;
    }
    #endif

    if (tree->teardown)
        tree->teardown(tree);
    tree->teardown = NULL;

    #if MPACK_SETJMP
    if (tree->jump_env)
        MPACK_FREE(tree->jump_env);
    tree->jump_env = NULL;
    #endif

    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_log("tree %p setting error %i: %s\n", tree, (int)error, mpack_error_to_string(error));

    if (tree->error == mpack_ok) {
        tree->error = error;
        #if MPACK_SETJMP
        if (tree->jump_env)
            longjmp(*tree->jump_env, 1);
        #endif
    }
}

void mpack_node_flag_error(mpack_node_t* node, mpack_error_t error) {
    mpack_tree_flag_error(node->tree, error);
}

#if MPACK_DEBUG && MPACK_STDIO && MPACK_SETJMP && !MPACK_NO_PRINT
static void mpack_node_print_element(mpack_node_t* node, size_t depth) {
    mpack_tag_t val = node->tag;
    switch (val.type) {

        case mpack_type_nil:
            printf("null");
            break;
        case mpack_type_bool:
            printf(val.v.b ? "true" : "false");
            break;

        case mpack_type_float:
            printf("%f", val.v.f);
            break;
        case mpack_type_double:
            printf("%f", val.v.d);
            break;

        case mpack_type_int:
            printf("%"PRIi64, val.v.i);
            break;
        case mpack_type_uint:
            printf("%"PRIu64, val.v.u);
            break;

        case mpack_type_bin:
            printf("<binary data of length %u>", val.v.l);
            break;

        case mpack_type_ext:
            printf("<ext data of type %i and length %u>", val.exttype, val.v.l);
            break;

        case mpack_type_str:
            {
                putchar('"');
                const char* data = mpack_node_data(node);
                for (size_t i = 0; i < val.v.l; ++i) {
                    char c = data[i];
                    switch (c) {
                        case '\n': printf("\\n"); break;
                        case '\\': printf("\\\\"); break;
                        case '"': printf("\\\""); break;
                        default: putchar(c); break;
                    }
                }
                putchar('"');
            }
            break;

        case mpack_type_array:
            printf("[\n");
            for (size_t i = 0; i < val.v.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    printf("    ");
                mpack_node_print_element(mpack_node_array_at(node, i), depth + 1);
                if (i != val.v.n - 1)
                    putchar(',');
                putchar('\n');
            }
            for (size_t i = 0; i < depth; ++i)
                printf("    ");
            putchar(']');
            break;

        case mpack_type_map:
            printf("{\n");
            for (size_t i = 0; i < val.v.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    printf("    ");
                mpack_node_print_element(mpack_node_map_key_at(node, i), depth + 1);
                printf(": ");
                mpack_node_print_element(mpack_node_map_value_at(node, i), depth + 1);
                if (i != val.v.n - 1)
                    putchar(',');
                putchar('\n');
            }
            for (size_t i = 0; i < depth; ++i)
                printf("    ");
            putchar('}');
            break;
    }
}

void mpack_node_print(mpack_node_t* node) {
    int depth = 2;
    for (int i = 0; i < depth; ++i)
        printf("    ");
    mpack_node_print_element(node, depth);
    putchar('\n');
}
#endif



/*
 * Node Data Functions
 */

size_t mpack_node_copy_data(mpack_node_t* node, char* buffer, size_t size) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_type_t type = node->tag.type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    if (node->tag.v.l > size) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    mpack_memcpy(buffer, node->data.bytes, node->tag.v.l);
    return (size_t)node->tag.v.l;
}

void mpack_node_copy_cstr(mpack_node_t* node, char* buffer, size_t size) {
    if (mpack_node_error(node) != mpack_ok)
        return;

    mpack_assert(size >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    if (node->tag.type != mpack_type_str) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    if (node->tag.v.l > size - 1) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_too_big);
        return;
    }

    mpack_memcpy(buffer, node->data.bytes, node->tag.v.l);
    buffer[node->tag.v.l] = '\0';
}

#ifdef MPACK_MALLOC
char* mpack_node_data_alloc(mpack_node_t* node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure this is a valid data type
    mpack_type_t type = node->tag.type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node->tag.v.l > maxlen) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)node->tag.v.l);
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node->data.bytes, node->tag.v.l);
    return ret;
}

char* mpack_node_cstr_alloc(mpack_node_t* node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure maxlen makes sense
    if (maxlen < 1) {
        mpack_break("maxlen is zero; you must have room for at least a null-terminator");
        mpack_node_flag_error(node, mpack_error_bug);
        return NULL;
    }

    if (node->tag.type != mpack_type_str) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node->tag.v.l > maxlen - 1) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node->tag.v.l + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node->data.bytes, node->tag.v.l);
    ret[node->tag.v.l] = '\0';
    return ret;
}
#endif


/*
 * Compound Node Functions
 */

mpack_node_t* mpack_node_map_int_impl(mpack_node_t* node, int64_t num, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    for (size_t i = 0; i < node->tag.v.n; ++i) {
        mpack_node_t* key = mpack_node_child(node, i * 2);
        mpack_node_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->tag.type == mpack_type_int && key->tag.v.i == num)
            return value;
        if (key->tag.type == mpack_type_uint && num >= 0 && key->tag.v.u == (uint64_t)num)
            return value;
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

mpack_node_t* mpack_node_map_uint_impl(mpack_node_t* node, uint64_t num, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    for (size_t i = 0; i < node->tag.v.n; ++i) {
        mpack_node_t* key = mpack_node_child(node, i * 2);
        mpack_node_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->tag.type == mpack_type_uint && key->tag.v.u == num)
            return value;
        if (key->tag.type == mpack_type_int && key->tag.v.i >= 0 && (uint64_t)key->tag.v.i == num)
            return value;
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

mpack_node_t* mpack_node_map_str_impl(mpack_node_t* node, const char* str, size_t length, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    for (size_t i = 0; i < node->tag.v.n; ++i) {
        mpack_node_t* key = mpack_node_child(node, i * 2);
        mpack_node_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->tag.type == mpack_type_str && key->tag.v.l == length && mpack_memcmp(str, key->data.bytes, length) == 0)
            return value;
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

bool mpack_node_map_contains_str(mpack_node_t* node, const char* str, size_t length) {
    if (mpack_node_error(node) != mpack_ok)
        return false;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return false;
    }

    for (size_t i = 0; i < node->tag.v.n; ++i) {
        mpack_node_t* key = mpack_node_child(node, i * 2);
        if (key->tag.type == mpack_type_str && key->tag.v.l == length && mpack_memcmp(str, key->data.bytes, length) == 0)
            return true;
    }

    return false;
}


#endif


