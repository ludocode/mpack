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

static mpack_node_t* mpack_tree_node_at(mpack_tree_t* tree, size_t index) {
    mpack_assert(tree->error == mpack_ok, "cannot fetch node from tree in error state %s",
            mpack_error_to_string(tree->error));

    #ifdef MPACK_MALLOC
    if (tree->pages) {
        mpack_assert(index < tree->page_count * MPACK_NODE_PAGE_SIZE,
                "cannot fetch node at index %i, tree only has %i nodes",
                (int)index, (int)tree->page_count * MPACK_NODE_PAGE_SIZE);
        return &tree->pages[index / MPACK_NODE_PAGE_SIZE][index % MPACK_NODE_PAGE_SIZE];
    }
    #endif

    mpack_assert(index < tree->node_count, "cannot fetch node at index %i, tree only has %i nodes",
            (int)index, (int)tree->node_count);
    return &tree->pool[index];
}

static inline mpack_node_t* mpack_node_child(mpack_node_t* node, size_t child) {
    return mpack_tree_node_at(node->tree, node->data.children + child);
}

/*
 * Tree Functions
 */

mpack_node_t* mpack_tree_root(mpack_tree_t* tree) {
    if (tree->error != mpack_ok)
        return &tree->nil_node;
    return mpack_tree_node_at(tree, 0);
}

// note: the tree parsing functions flag errors on the reader, not on the
// tree. the tree picks up the reader's error state when done.
static void mpack_tree_read_node(mpack_tree_t* tree, mpack_node_t* node,
        mpack_reader_t* reader, int depth, size_t* possible_nodes_left) {
    mpack_memset(&node->tag, 0, sizeof(node->tag));
    node->tree = tree;

    size_t pos = reader->pos;
    node->tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    mpack_type_t type = node->tag.type;

    // read a tag, keeping track of the number of possible nodes left. (one
    // byte has already been counted for each node.)
    if (reader->pos - pos - 1 > *possible_nodes_left) {
        mpack_reader_flag_error(reader, mpack_error_invalid);
        return;
    }
    *possible_nodes_left -= reader->pos - pos - 1;

    if (type == mpack_type_array || type == mpack_type_map) {
        if (depth + 1 == MPACK_NODE_MAX_DEPTH) {
            mpack_reader_flag_error(reader, mpack_error_too_big);
            return;
        }

        uint64_t total64 = (type == mpack_type_map) ? node->tag.v.n * 2 : node->tag.v.n;
        if (total64 > SIZE_MAX) {
            mpack_tree_flag_error(tree, mpack_error_too_big);
            return;
        }
        size_t total = (size_t)total64;
        node->data.children = tree->node_count;
        tree->node_count += total;

        // each node is at least one byte. count these bytes now to ensure that
        // malicious nested data is not trying to make us allocate more nodes
        // than there are bytes in the data.
        if (total > *possible_nodes_left) {
            mpack_reader_flag_error(reader, mpack_error_invalid);
            return;
        }
        *possible_nodes_left -= total;

        // make sure we have enough nodes
        #ifdef MPACK_MALLOC
        if (tree->pages) {
            while (tree->node_count > tree->page_count * MPACK_NODE_PAGE_SIZE) {

                // grow page array if needed
                if (tree->page_count == tree->page_capacity) {
                    size_t new_capacity = tree->page_capacity * 2;
                    mpack_node_t** new_pages = (mpack_node_t**)MPACK_MALLOC(sizeof(mpack_node_t*) * new_capacity);
                    if (new_pages == NULL) {
                        mpack_tree_flag_error(tree, mpack_error_memory);
                        return;
                    }
                    mpack_memcpy(new_pages, tree->pages, tree->page_count * sizeof(mpack_node_t*));
                    MPACK_FREE(tree->pages);
                    tree->pages = new_pages;
                    tree->page_capacity = new_capacity;
                }

                // allocate new page
                tree->pages[tree->page_count] = (mpack_node_t*)MPACK_MALLOC(sizeof(mpack_node_t) * MPACK_NODE_PAGE_SIZE);
                if (tree->pages[tree->page_count] == NULL) {
                    mpack_tree_flag_error(tree, mpack_error_memory);
                    return;
                }
                ++tree->page_count;

            }
        } else
        #endif
        {
            if (tree->node_count > tree->pool_count) {
                tree->node_count = tree->pool_count;
                mpack_tree_flag_error(tree, mpack_error_too_big);
                return;
            }
        }

        // read the nodes
        for (size_t i = 0; i < total; ++i)
            mpack_tree_read_node(tree, mpack_node_child(node, i), reader, depth + 1, possible_nodes_left);

        if (type == mpack_type_array)
            mpack_done_array(reader);
        else
            mpack_done_map(reader);

    } else if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext) {
        size_t total = node->tag.v.l;

        if (total > *possible_nodes_left) {
            mpack_reader_flag_error(reader, mpack_error_invalid);
            return;
        }
        *possible_nodes_left -= total;

        node->data.bytes = mpack_read_bytes_inplace(reader, total);

        if (type == mpack_type_str)
            mpack_done_str(reader);
        else if (type == mpack_type_bin)
            mpack_done_bin(reader);
        else if (type == mpack_type_ext)
            mpack_done_ext(reader);
    }
}

void mpack_tree_init_clear(mpack_tree_t* tree) {
    mpack_memset(tree, 0, sizeof(*tree));
    tree->nil_node.tree = tree;
    tree->nil_node.tag.type = mpack_type_nil;
}

void mpack_tree_parse(mpack_tree_t* tree, const char* data, size_t length) {
    if (length == 0) {
        mpack_tree_init_error(tree, mpack_error_io);
        return;
    }

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, length);

    size_t possible_nodes_left = length - 1;
    tree->node_count = 1;
    mpack_tree_read_node(tree, mpack_tree_node_at(tree, 0), &reader, 0, &possible_nodes_left);

    tree->size = length - mpack_reader_remaining(&reader, NULL);
    tree->error = mpack_reader_destroy(&reader);
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

void mpack_tree_init_nodes(mpack_tree_t* tree, const char* data, size_t length, mpack_node_t* node_pool, size_t node_pool_count) {
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

static void mpack_file_tree_teardown(void* context) {
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*)context;
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
        tree->teardown(tree->context);
    tree->teardown = NULL;
    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_log("tree %p setting error %i: %s\n", tree, (int)error, mpack_error_to_string(error));

    if (!tree->error) {
        tree->error = error;
        #if MPACK_SETJMP
        if (tree->jump)
            longjmp(tree->jump_env, 1);
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



/**
 * Node Primitive Value Functions
 */

void mpack_node_nil(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return;
    if (node->tag.type != mpack_type_nil)
      mpack_node_flag_error(node, mpack_error_type);
}

bool mpack_node_bool(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return false;

    if (node->tag.type == mpack_type_bool)
        return node->tag.v.b;

    mpack_node_flag_error(node, mpack_error_type);
    return false;
}

void mpack_node_true(mpack_node_t* node) {
    if (mpack_node_bool(node) != true)
        mpack_node_flag_error(node, mpack_error_type);
}

void mpack_node_false(mpack_node_t* node) {
    if (mpack_node_bool(node) != false)
        mpack_node_flag_error(node, mpack_error_type);
}

uint8_t mpack_node_u8(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= UINT8_MAX)
            return (uint8_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= 0 && node->tag.v.i <= UINT8_MAX)
            return (uint8_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint16_t mpack_node_u16(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= UINT16_MAX)
            return (uint16_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= 0 && node->tag.v.i <= UINT16_MAX)
            return (uint16_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint32_t mpack_node_u32(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= UINT32_MAX)
            return (uint32_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= 0 && node->tag.v.i <= UINT32_MAX)
            return (uint32_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint64_t mpack_node_u64(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint)
        return node->tag.v.u;
    else if (node->tag.type == mpack_type_int)
        return (uint64_t)node->tag.v.i;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int8_t mpack_node_i8(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= INT8_MAX)
            return (int8_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= INT8_MIN && node->tag.v.i <= INT8_MAX)
            return (int8_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int16_t mpack_node_i16(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= INT16_MAX)
            return (int16_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= INT16_MIN && node->tag.v.i <= INT16_MAX)
            return (int16_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int32_t mpack_node_i32(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= INT32_MAX)
            return (int32_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= INT32_MIN && node->tag.v.i <= INT32_MAX)
            return (int32_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int64_t mpack_node_i64(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        if (node->tag.v.u <= (uint64_t)INT64_MAX)
            return (int64_t)node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        return node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

float mpack_node_float(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0.0f;

    if (node->tag.type == mpack_type_uint)
        return (float)node->tag.v.u;
    else if (node->tag.type == mpack_type_int)
        return (float)node->tag.v.i;
    else if (node->tag.type == mpack_type_float)
        return node->tag.v.f;
    else if (node->tag.type == mpack_type_double)
        return (float)node->tag.v.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0f;
}

double mpack_node_double(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0.0;

    if (node->tag.type == mpack_type_uint)
        return (double)node->tag.v.u;
    else if (node->tag.type == mpack_type_int)
        return (double)node->tag.v.i;
    else if (node->tag.type == mpack_type_float)
        return (double)node->tag.v.f;
    else if (node->tag.type == mpack_type_double)
        return node->tag.v.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0;
}

float mpack_node_float_strict(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0.0f;

    if (node->tag.type == mpack_type_float)
        return node->tag.v.f;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0f;
}

double mpack_node_double_strict(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0.0;

    if (node->tag.type == mpack_type_float)
        return (double)node->tag.v.f;
    else if (node->tag.type == mpack_type_double)
        return node->tag.v.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0;
}



/*
 * Node Data Functions
 */

int8_t mpack_node_exttype(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_ext)
        return node->tag.exttype;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

size_t mpack_node_data_len(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    mpack_type_t type = node->tag.type;
    if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext)
        return (size_t)node->tag.v.l;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

size_t mpack_node_strlen(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_str)
        return (size_t)node->tag.v.l;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

const char* mpack_node_data(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return NULL;

    mpack_type_t type = node->tag.type;
    if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext)
        return node->data.bytes;

    mpack_node_flag_error(node, mpack_error_type);
    return NULL;
}

size_t mpack_node_copy_data(mpack_node_t* node, char* buffer, size_t size) {
    if (node->tree->error != mpack_ok)
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
    if (node->tree->error != mpack_ok)
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
    if (node->tree->error != mpack_ok)
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
    if (node->tree->error != mpack_ok)
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

size_t mpack_node_array_length(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return (size_t)node->tag.v.n;
}

mpack_node_t* mpack_node_array_at(mpack_node_t* node, size_t index) {
    if (node->tree->error != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    if (index >= node->tag.v.n) {
        mpack_node_flag_error(node, mpack_error_data);
        return &node->tree->nil_node;
    }

    return mpack_node_child(node, index);
}

static mpack_node_t* mpack_node_map_at(mpack_node_t* node, size_t index, size_t offset) {
    if (node->tree->error != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    if (index >= node->tag.v.n) {
        mpack_node_flag_error(node, mpack_error_data);
        return &node->tree->nil_node;
    }

    return mpack_node_child(node, index * 2 + offset);
}

size_t mpack_node_map_count(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return node->tag.v.n;
}

mpack_node_t* mpack_node_map_key_at(mpack_node_t* node, size_t index) {
    return mpack_node_map_at(node, index, 0);
}

mpack_node_t* mpack_node_map_value_at(mpack_node_t* node, size_t index) {
    return mpack_node_map_at(node, index, 1);
}

mpack_node_t* mpack_node_map_int(mpack_node_t* node, int64_t num) {
    if (node->tree->error != mpack_ok)
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

    mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

mpack_node_t* mpack_node_map_uint(mpack_node_t* node, uint64_t num) {
    if (node->tree->error != mpack_ok)
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

    mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

mpack_node_t* mpack_node_map_str(mpack_node_t* node, const char* str, size_t length) {
    if (node->tree->error != mpack_ok)
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

    mpack_node_flag_error(node, mpack_error_data);
    return &node->tree->nil_node;
}

mpack_node_t* mpack_node_map_cstr(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_str(node, cstr, mpack_strlen(cstr));
}

bool mpack_node_map_contains_str(mpack_node_t* node, const char* str, size_t length) {
    if (node->tree->error != mpack_ok)
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

bool mpack_node_map_contains_cstr(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_contains_str(node, cstr, mpack_strlen(cstr));
}


#endif


