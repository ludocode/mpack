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


/*
 * Tree Functions
 */

static void mpack_node_destroy(mpack_node_t* node) {
    mpack_type_t type = node->tag.type;
    if (type == mpack_type_array) {
        for (size_t i = 0; i < node->tag.v.u; ++i)
            mpack_node_destroy(&node->data.children[i]);
        MPACK_FREE(node->data.children);
    } else if (type == mpack_type_map) {
        for (size_t i = 0; i < node->tag.v.u * 2; ++i)
            mpack_node_destroy(&node->data.children[i]);
        MPACK_FREE(node->data.children);
    }
}

// note: the tree parsing functions flag errors on the reader, not on the
// tree. the tree picks up the reader's error state when done.
static void mpack_tree_read_node(mpack_tree_t* tree, mpack_node_t* node, mpack_reader_t* reader, int depth) {
    node->tree = tree;

    node->tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    mpack_type_t type = node->tag.type;

    if (type == mpack_type_array || type == mpack_type_map) {
        if (depth + 1 == MPACK_NODE_MAX_DEPTH) {
            mpack_reader_flag_error(reader, mpack_error_too_big);
            return;
        }

        // If a node declares a large number of children, we allocate its nodes in
        // a growable array instead of just allocating the declared number right
        // away. This costs some performance, but we can't trust that the data is
        // actually valid, so we don't want to be allocating gigs of blank space
        // for a maliciously crafted chunk of MessagePack. Otherwise, repeating
        // 0xDE 0xFF 0xFF for example would cause explosive memory growth.
        //
        // This limit combined with the depth limit means malicious data shouldn't
        // be able to force allocation of more than a couple megs before failing.
        // (This will also safely handle an allocation failure on more memory-limited
        // platforms provided malloc() can actually fail, but it still seems like
        // there should be a better way to handle this, so I'm open to suggestions.
        // Probably we should just do the simplest thing which is to allow setting
        // a hard limit on the number of nodes, but this shouldn't be the default
        // API, C programmer's disease and all.)

        // TODO: make sure all these overflow checks are correct...

        uint64_t total64 = (type == mpack_type_map) ? node->tag.v.u * 2 : node->tag.v.u;
        if (total64 > SIZE_MAX) {
            mpack_tree_flag_error(tree, mpack_error_too_big);
            return;
        }
        size_t total = (size_t)total64;
        size_t count = MPACK_NODE_ARRAY_STARTING_SIZE;
        if (count > total)
            count = total;

        mpack_node_t* children = (mpack_node_t*) MPACK_MALLOC(count * sizeof(mpack_node_t));
        if (children == NULL) {
            node->tag.type = mpack_type_nil;
            mpack_reader_flag_error(reader, mpack_error_memory);
            return;
        }

        for (size_t i = 0; i < total; ++i) {
            if (i == count) {
                size_t new_count = count * 2;
                if (new_count > total || new_count / 2 != count) // overflow check
                    new_count = total;

                mpack_node_t* new_children = (mpack_node_t*) MPACK_MALLOC(new_count * sizeof(mpack_node_t));
                if (new_children == NULL) {
                    for (size_t j = 0; j < count; ++j)
                        mpack_node_destroy(&children[j]);
                    MPACK_FREE(children);
                    node->tag.type = mpack_type_nil;
                    mpack_reader_flag_error(reader, mpack_error_memory);
                    return;
                }

                memcpy(new_children, children, count * sizeof(mpack_node_t));
                MPACK_FREE(children);
                children = new_children;
                count = new_count;
            }

            mpack_tree_read_node(tree, &children[i], reader, depth + 1);
        }

        node->data.children = children;

        if (type == mpack_type_array)
            mpack_done_array(reader);
        else
            mpack_done_map(reader);

    } else if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext) {
        node->data.bytes = reader->buffer + reader->pos;
        mpack_skip_bytes(reader, node->tag.v.u);

        if (type == mpack_type_str)
            mpack_done_str(reader);
        else if (type == mpack_type_bin)
            mpack_done_bin(reader);
        else if (type == mpack_type_ext)
            mpack_done_ext(reader);
    }
}

void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length) {
    memset(tree, 0, sizeof(*tree));

    tree->nil_node.tree = tree;
    tree->nil_node.tag.type = mpack_type_nil;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, length);
    mpack_tree_read_node(tree, &tree->root, &reader, 0);

    tree->size = length - mpack_reader_remaining(&reader, NULL);
    tree->error = mpack_reader_destroy(&reader);
}

void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error) {
    memset(tree, 0, sizeof(*tree));

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

static bool mpack_file_tree_read(mpack_tree_t* tree, mpack_file_tree_t* file_tree, const char* filename, int max_size) {

    // open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }

    // get the file size
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size == -1) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }
    if (size == 0) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_invalid);
        return false;
    }
    if (max_size != 0 && size > max_size) {
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
    int total = 0;
    while (total < size) {
        int read = fread(file_tree->data + total, 1, size - total, file);
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

    // max_size is converted to int because the C STDIO family of file functions use int (e.g. ftell)
    if (max_size > INT_MAX) {
        mpack_assert(0, "max_size of %"PRIu64" is invalid, maximum is MAX_INT", (uint64_t)max_size);
        mpack_tree_init_error(tree, mpack_error_bug);
        return;
    }

    // allocate file tree
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*) MPACK_MALLOC(sizeof(mpack_file_tree_t));
    if (file_tree == NULL) {
        mpack_tree_init_error(tree, mpack_error_memory);
        return;
    }

    // read all data
    if (!mpack_file_tree_read(tree, file_tree, filename, (int)max_size)) {
        MPACK_FREE(file_tree);
        return;
    }

    mpack_tree_init(tree, file_tree->data, file_tree->size);
    mpack_tree_set_context(tree, file_tree);
    mpack_tree_set_teardown(tree, mpack_file_tree_teardown);
}
#endif

mpack_error_t mpack_tree_destroy(mpack_tree_t* tree) {
    mpack_node_destroy(&tree->root);
    if (tree->teardown)
        tree->teardown(tree->context);
    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_log("tree %p setting error %i: %s\n", tree, (int)error, mpack_error_to_string(error));

    if (!tree->error) {
        tree->error = error;
        if (tree->jump)
            longjmp(tree->jump_env, 1);
    }
}

void mpack_node_flag_error(mpack_node_t* node, mpack_error_t error) {
    mpack_tree_flag_error(node->tree, error);
}

#if MPACK_DEBUG && MPACK_STDIO && MPACK_SETJMP
void mpack_node_print_element(mpack_node_t* node, size_t depth) {
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
            printf("<binary data>");
            break;

        case mpack_type_ext:
            printf("<ext data of type %i>", val.exttype);
            break;

        case mpack_type_str:
            {
                putchar('"');
                const char* data = mpack_node_data(node);
                for (size_t i = 0; i < val.v.u; ++i) {
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
            for (size_t i = 0; i < val.v.u; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    printf("    ");
                mpack_node_print_element(mpack_node_array_at(node, i), depth + 1);
                if (i != val.v.u - 1)
                    putchar(',');
                putchar('\n');
            }
            for (size_t i = 0; i < depth; ++i)
                printf("    ");
            putchar(']');
            break;

        case mpack_type_map:
            printf("{\n");
            for (size_t i = 0; i < val.v.u; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    printf("    ");
                mpack_node_print_element(mpack_node_map_key_at(node, i), depth + 1);
                printf(": ");
                mpack_node_print_element(mpack_node_map_value_at(node, i), depth + 1);
                if (i != val.v.u - 1)
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
        if (node->tag.v.i >= INT64_MIN && node->tag.v.i <= INT64_MAX)
            return (int64_t)node->tag.v.i;
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
        return (size_t)node->tag.v.u;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

size_t mpack_node_strlen(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_str)
        return (size_t)node->tag.v.u;

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

    if (node->tag.v.u > size) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    memcpy(buffer, node->data.bytes, node->tag.v.u);
    return (size_t)node->tag.v.u;
}

void mpack_node_copy_cstr(mpack_node_t* node, char* buffer, size_t size) {
    if (node->tree->error != mpack_ok)
        return;

    // make sure buffer makes sense
    if (size < 1) {
        mpack_assert(0, "buffer size is zero; you must have room for at least a null-terminator");
        mpack_node_flag_error(node, mpack_error_bug);
        return;
    }

    if (node->tag.type != mpack_type_str) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    if (node->tag.v.u > size - 1) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_too_big);
        return;
    }

    memcpy(buffer, node->data.bytes, node->tag.v.u);
    buffer[node->tag.v.u] = '\0';
}

char* mpack_node_data_alloc(mpack_node_t* node, size_t maxlen) {
    if (node->tree->error != mpack_ok)
        return NULL;

    // make sure this is a valid data type
    mpack_type_t type = node->tag.type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node->tag.v.u > maxlen) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)node->tag.v.u);
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    memcpy(ret, node->data.bytes, node->tag.v.u);
    return ret;
}

char* mpack_node_cstr_alloc(mpack_node_t* node, size_t maxlen) {
    if (node->tree->error != mpack_ok)
        return NULL;

    // make sure maxlen makes sense
    if (maxlen < 1) {
        mpack_assert(0, "maxlen is zero; you must have room for at least a null-terminator");
        mpack_node_flag_error(node, mpack_error_bug);
        return NULL;
    }

    if (node->tag.type != mpack_type_str) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node->tag.v.u > maxlen - 1) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node->tag.v.u + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    memcpy(ret, node->data.bytes, node->tag.v.u);
    ret[node->tag.v.u] = '\0';
    return ret;
}


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

    return (size_t)node->tag.v.u;
}

mpack_node_t* mpack_node_array_at(mpack_node_t* node, size_t index) {
    if (node->tree->error != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    if (index >= node->tag.v.u) {
        mpack_node_flag_error(node, mpack_error_data);
        return &node->tree->nil_node;
    }

    return &node->data.children[index];
}

static mpack_node_t* mpack_node_map_at(mpack_node_t* node, size_t index, size_t offset) {
    if (node->tree->error != mpack_ok)
        return &node->tree->nil_node;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return &node->tree->nil_node;
    }

    if (index >= node->tag.v.u) {
        mpack_node_flag_error(node, mpack_error_data);
        return &node->tree->nil_node;
    }

    return &node->data.children[index * 2 + offset];
}

size_t mpack_node_map_count(mpack_node_t* node) {
    if (node->tree->error != mpack_ok)
        return 0;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return node->tag.v.u;
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

    for (size_t i = 0; i < node->tag.v.u; ++i) {
        mpack_node_t* key = &node->data.children[i * 2];
        mpack_node_t* value = &node->data.children[i * 2 + 1];

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

    for (size_t i = 0; i < node->tag.v.u; ++i) {
        mpack_node_t* key = &node->data.children[i * 2];
        mpack_node_t* value = &node->data.children[i * 2 + 1];

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

    for (size_t i = 0; i < node->tag.v.u; ++i) {
        mpack_node_t* key = &node->data.children[i * 2];
        mpack_node_t* value = &node->data.children[i * 2 + 1];

        if (key->tag.type == mpack_type_str && key->tag.v.u == length && memcmp(str, key->data.bytes, length) == 0)
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

    for (size_t i = 0; i < node->tag.v.u; ++i) {
        mpack_node_t* key = &node->data.children[i * 2];
        if (key->tag.type == mpack_type_str && key->tag.v.u == length && memcmp(str, key->data.bytes, length) == 0)
            return true;
    }

    return false;
}

bool mpack_node_map_contains_cstr(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_contains_str(node, cstr, mpack_strlen(cstr));
}


#endif


