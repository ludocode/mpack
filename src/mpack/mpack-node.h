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

/**
 * @file
 *
 * Declares the MPack dynamic Node API.
 */

#ifndef MPACK_NODE_H
#define MPACK_NODE_H 1

#include "mpack-reader.h"

#if MPACK_NODE

#if !MPACK_READER
#error "MPACK_NODE requires MPACK_READER."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup node Node API
 *
 * The MPack Node API allows you to parse a chunk of MessagePack data
 * in-place into a dynamically typed data structure.
 *
 * @{
 */

/**
 * A node in a parsed MPack tree.
 *
 * Nodes represent either primitive values or compound types. If a
 * node is a compound type, it contains links to its child nodes, or
 * a pointer to its underlying data.
 *
 * Nodes are immutable.
 */
typedef struct mpack_node_t mpack_node_t;

struct mpack_node_t {
    mpack_tag_t tag;
    struct mpack_tree_t* tree;
    union {
        const char* bytes;
        mpack_node_t* children;
    } data;
};

/**
 * An MPack tree parsed from a blob of MessagePack.
 *
 * The tree contains a single root node which contains all parsed data.
 * The tree and its nodes are immutable.
 */
typedef struct mpack_tree_t mpack_tree_t;

/**
 * A teardown function to be called when the tree is destroyed.
 */
typedef void (*mpack_tree_teardown_t)(mpack_tree_t* tree);

/*
 * mpack_tree_link_t forms a linked list of node pages. It is allocated
 * separately from the page so that we can store the first link internally
 * without a malloc (the only link in a pooled tree), and we don't
 * affect the size of page pools or violate strict aliasing.
 */
typedef struct mpack_tree_link_t {
    struct mpack_tree_link_t* next;
    mpack_node_t* nodes;
    size_t pos;
    size_t left;
} mpack_tree_link_t;

struct mpack_tree_t {
    mpack_tree_teardown_t teardown; /* Function to teardown the context on destroy */
    void* context;                  /* Context for tree callbacks */

    mpack_node_t nil_node; /* a nil node to be returned in case of error */
    mpack_error_t error;

    size_t node_count;
    size_t size;
    mpack_node_t* root;

    mpack_tree_link_t page;
    #ifdef MPACK_MALLOC
    bool owned;
    #endif

    #if MPACK_SETJMP
    /* Optional jump target in case of error (pointer because it's
     * very large and may be unused) */
    jmp_buf* jump_env;
    #endif
};

// internal node child lookups

static inline mpack_node_t* mpack_node_child(mpack_node_t* node, size_t child) {
    return node->data.children + child;
}

/**
 * @name Tree Functions
 * @{
 */

#ifdef MPACK_MALLOC
/**
 * Initializes a tree by parsing the given data buffer. The tree must be destroyed
 * with mpack_tree_destroy(), even if parsing fails.
 *
 * The tree will allocate pages of nodes as needed, and free them when destroyed.
 *
 * Any string or blob data types reference the original data, so the data
 * pointer must remain valid until after the tree is destroyed.
 */
void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length);
#endif

/**
 * Initializes a tree by parsing the given data buffer, using the given
 * node pool to store the results.
 *
 * If the data does not fit in the pool, mpack_error_too_big will be flagged
 * on the tree.
 *
 * The tree must be destroyed with mpack_tree_destroy(), even if parsing fails.
 */
void mpack_tree_init_pool(mpack_tree_t* tree, const char* data, size_t length, mpack_node_t* node_pool, size_t node_pool_count);

/**
 * Initializes an mpack tree directly into an error state. Use this if you
 * are writing a wrapper to mpack_tree_init() which can fail its setup.
 */
void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error);

#if MPACK_STDIO
/**
 * Initializes a tree by reading and parsing the given file. The tree must be
 * destroyed with mpack_tree_destroy(), even if parsing fails.
 *
 * The file is opened, loaded fully into memory, and closed before this call
 * returns.
 *
 * @param tree The tree to initialize
 * @param filename The filename passed to fopen() to read the file
 * @param max_bytes The maximum size of file to load, or 0 for unlimited size.
 */
void mpack_tree_init_file(mpack_tree_t* tree, const char* filename, size_t max_bytes);
#endif

/**
 * Returns the root node of the tree, if the tree is not in an error state.
 * Returns a nil node otherwise.
 */
mpack_node_t* mpack_tree_root(mpack_tree_t* tree);

/**
 * Returns the error state of the tree.
 */
static inline mpack_error_t mpack_tree_error(mpack_tree_t* tree) {
    return tree->error;
}

/**
 * Returns the number of bytes used in the buffer when the tree was
 * parsed. If there is something in the buffer after the MessagePack
 * object (such as another object), this can be used to find it.
 */
static inline size_t mpack_tree_size(mpack_tree_t* tree) {
    return tree->size;
}

/**
 * Destroys the tree.
 */
mpack_error_t mpack_tree_destroy(mpack_tree_t* tree);

/**
 * Sets the custom pointer to pass to the tree callbacks, such as teardown.
 *
 * @param tree The MPack tree.
 * @param context User data to pass to the tree callbacks.
 */
static inline void mpack_tree_set_context(mpack_tree_t* tree, void* context) {
    tree->context = context;
}

/**
 * Sets the teardown function to call when the tree is destroyed.
 *
 * This should normally be used with mpack_tree_set_context() to register
 * a custom pointer to pass to the teardown function.
 *
 * @param tree The MPack tree.
 * @param teardown The function to call when the tree is destroyed.
 */
static inline void mpack_tree_set_teardown(mpack_tree_t* tree, mpack_tree_teardown_t teardown) {
    tree->teardown = teardown;
}

#if MPACK_SETJMP

/**
 * @hideinitializer
 *
 * Registers a jump target in case of error.
 *
 * If the tree is in an error state, 1 is returned when this is called. Otherwise
 * 0 is returned when this is called, and when the first error occurs, control flow
 * will jump to the point where this was called, resuming as though it returned 1.
 * This ensures an error handling block runs exactly once in case of error.
 *
 * A tree that jumps still needs to be destroyed. You must call
 * mpack_tree_destroy() in your jump handler after getting the final error state.
 *
 * The argument may be evaluated multiple times.
 *
 * @returns 0 if the tree is not in an error state; 1 if and when an error occurs.
 * @see mpack_tree_destroy()
 */
#define MPACK_TREE_SETJMP(tree)                                          \
    (mpack_assert((tree)->jump_env == NULL, "already have a jump set!"), \
    ((tree)->error != mpack_ok) ? 1 :                                    \
        !((tree)->jump_env = (jmp_buf*)MPACK_MALLOC(sizeof(jmp_buf))) ?  \
            ((tree)->error = mpack_error_memory, 1) :                    \
            (setjmp(*(tree)->jump_env)))

/**
 * Clears a jump target. Subsequent tree reading errors will not cause a jump.
 */
static inline void mpack_tree_clearjmp(mpack_tree_t* tree) {
    if (tree->jump_env)
        MPACK_FREE(tree->jump_env);
    tree->jump_env = NULL;
}
#endif

/**
 * Places the tree in the given error state, jumping if a jump target is set.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you read it.
 *
 * If the tree is already in an error state, this call is ignored and no jump
 * is performed.
 */
void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error);

/**
 * Places the node's tree in the given error state, jumping if a jump target is set.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you read it.
 *
 * If the tree is already in an error state, this call is ignored and no jump
 * is performed.
 */
void mpack_node_flag_error(mpack_node_t* node, mpack_error_t error);

/**
 * @}
 */

/**
 * @name Node Core Functions
 * @{
 */

/**
 * Returns the error state of the node's tree.
 */
static inline mpack_error_t mpack_node_error(mpack_node_t* node) {
    return mpack_tree_error(node->tree);
}

/**
 * Returns the tag contained by the given node.
 */
static inline mpack_tag_t mpack_node_tag(mpack_node_t* node) {
    return node->tag;
}

#if MPACK_DEBUG && MPACK_STDIO && MPACK_SETJMP && !MPACK_NO_PRINT
/**
 * Converts a node to JSON and pretty-prints it to stdout.
 *
 * This function is only available in debugging mode.
 */
void mpack_node_print(mpack_node_t* node);
#endif

/**
 * @}
 */

/**
 * @name Node Primitive Value Functions
 * @{
 */

/**
 * Returns the type of the node.
 */
static inline mpack_type_t mpack_node_type(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_type_nil;
    return node->tag.type;
}

/**
 * Checks if the given node is of nil type, raising mpack_error_type otherwise.
 */
static inline void mpack_node_nil(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    if (node->tag.type != mpack_type_nil)
        mpack_node_flag_error(node, mpack_error_type);
}

/**
 * Returns the bool value of the node. If this node is not of the correct
 * type, mpack_error_type is raised, and the return value should be discarded.
 */
static inline bool mpack_node_bool(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return false;

    if (node->tag.type == mpack_type_bool)
        return node->tag.v.b;

    mpack_node_flag_error(node, mpack_error_type);
    return false;
}

/**
 * Checks if the given node is of bool type with value true, raising
 * mpack_error_type otherwise.
 */
static inline void mpack_node_true(mpack_node_t* node) {
    if (mpack_node_bool(node) != true)
        mpack_node_flag_error(node, mpack_error_type);
}

/**
 * Checks if the given node is of bool type with value false, raising
 * mpack_error_type otherwise.
 */
static inline void mpack_node_false(mpack_node_t* node) {
    if (mpack_node_bool(node) != false)
        mpack_node_flag_error(node, mpack_error_type);
}

/**
 * Returns the 8-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline uint8_t mpack_node_u8(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 8-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline int8_t mpack_node_i8(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 16-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline uint16_t mpack_node_u16(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 16-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline int16_t mpack_node_i16(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 32-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline uint32_t mpack_node_u32(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 32-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline int32_t mpack_node_i32(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the 64-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline uint64_t mpack_node_u64(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_uint) {
        return node->tag.v.u;
    } else if (node->tag.type == mpack_type_int) {
        if (node->tag.v.i >= 0)
            return (uint64_t)node->tag.v.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

/**
 * Returns the 64-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
static inline int64_t mpack_node_i64(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the float value of the node. The underlying value can be an
 * integer, float or double; the value is converted to a float.
 *
 * Note that reading a double or a large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
static inline float mpack_node_float(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the double value of the node. The underlying value can be an
 * integer, float or double; the value is converted to a double.
 *
 * Note that reading a very large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
static inline double mpack_node_double(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the float value of the node. The underlying value must be a float,
 * not a double or an integer. This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float.
 */
static inline float mpack_node_float_strict(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0f;

    if (node->tag.type == mpack_type_float)
        return node->tag.v.f;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0f;
}

/**
 * Returns the double value of the node. The underlying value must be a float
 * or double, not an integer. This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float or double.
 */
static inline double mpack_node_double_strict(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0;

    if (node->tag.type == mpack_type_float)
        return (double)node->tag.v.f;
    else if (node->tag.type == mpack_type_double)
        return node->tag.v.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0;
}

/**
 * @}
 */

/**
 * @name Node Data Functions
 * @{
 */

/**
 * Returns the extension type of the given ext node.
 */
static inline int8_t mpack_node_exttype(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_ext)
        return node->tag.exttype;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

/**
 * Returns the length of the given str, bin or ext node.
 */
static inline size_t mpack_node_data_len(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_type_t type = node->tag.type;
    if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext)
        return (size_t)node->tag.v.l;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

/**
 * Returns the length in bytes of the given string node. This does not
 * include any null-terminator.
 */
static inline size_t mpack_node_strlen(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node->tag.type == mpack_type_str)
        return (size_t)node->tag.v.l;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

/**
 * Returns a pointer to the data contained by this node.
 *
 * Note that strings are not null-terminated! Use mpack_node_copy_cstr() or
 * mpack_node_cstr_alloc() to get a null-terminated string.
 *
 * The pointer is valid as long as the data backing the tree is valid.
 *
 * If this node is not of a str, bin or map, mpack_error_type is raised, and
 * NULL is returned.
 */
static inline const char* mpack_node_data(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    mpack_type_t type = node->tag.type;
    if (type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext)
        return node->data.bytes;

    mpack_node_flag_error(node, mpack_error_type);
    return NULL;
}

/**
 * Copies the bytes contained by this node into the given buffer, returning the
 * number of bytes in the node.
 *
 * If this node is not of a str, bin or map, mpack_error_type is raised, and the
 * buffer and return value should be discarded. If the node's data does not fit
 * in the given buffer, mpack_error_data is raised, and the buffer and return value
 * should be discarded.
 *
 * @param node The string node from which to copy data
 * @param buffer A buffer in which to copy the node's bytes
 * @param size The size of the given buffer
 */
size_t mpack_node_copy_data(mpack_node_t* node, char* buffer, size_t size);

/**
 * Copies the bytes contained by this string node into the given buffer and adds
 * a null terminator. If this node is not of a string type, mpack_error_type is raised,
 * and the buffer should be discarded. If the string does not fit, mpack_error_data is
 * raised, and the buffer should be discarded.
 *
 * If this node is not of a string type, mpack_error_type is raised, and the
 * buffer and return value should be discarded. If the string and null-terminator
 * do not fit in the given buffer, mpack_error_data is raised, and the buffer and
 * return value should be discarded.
 *
 * @param node The string node from which to copy data
 * @param buffer A buffer in which to copy the node's string
 * @param size The size of the given buffer
 */
void mpack_node_copy_cstr(mpack_node_t* node, char* buffer, size_t size);

#ifdef MPACK_MALLOC
/**
 * Allocates a new chunk of data using MPACK_MALLOC with the bytes
 * contained by this node. The returned string should be freed with MPACK_FREE.
 *
 * If this node is not a str, bin or ext type, mpack_error_type is raised
 * and the return value should be discarded. If the string and null-terminator
 * are longer than the given maximum length, mpack_error_too_big is raised, and
 * the return value should be discarded. If an allocation failure occurs,
 * mpack_error_memory is raised and the return value should be discarded.
 */
char* mpack_node_data_alloc(mpack_node_t* node, size_t maxlen);

/**
 * Allocates a new null-terminated string using MPACK_MALLOC with the string
 * contained by this node. The returned string should be freed with MPACK_FREE.
 *
 * If this node is not a string type, mpack_error_type is raised, and the return
 * value should be discarded.
 */
char* mpack_node_cstr_alloc(mpack_node_t* node, size_t maxlen);
#endif

/**
 * @}
 */

/**
 * @name Compound Node Functions
 * @{
 */

// internal implementation of map key lookup functions to support optional flag
mpack_node_t* mpack_node_map_str_impl(mpack_node_t* node, const char* str, size_t length, bool optional);
mpack_node_t* mpack_node_map_int_impl(mpack_node_t* node, int64_t num, bool optional);
mpack_node_t* mpack_node_map_uint_impl(mpack_node_t* node, uint64_t num, bool optional);

/**
 * Returns the length of the given array node. Raises mpack_error_type
 * and returns 0 if the given node is not an array.
 */
static inline size_t mpack_node_array_length(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node->tag.type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return (size_t)node->tag.v.n;
}

/**
 * Returns the node in the given array at the given index. If the node
 * is not an array, mpack_error_type is raised and a nil node is returned.
 * If the given index is out of bounds, mpack_error_data is raised and
 * a nil node is returned.
 */
static inline mpack_node_t* mpack_node_array_at(mpack_node_t* node, size_t index) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the number of key/value pairs in the given map node. Raises
 * mpack_error_type and returns 0 if the given node is not a map.
 */
static inline size_t mpack_node_map_count(mpack_node_t* node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node->tag.type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return node->tag.v.n;
}

// internal node map lookup
static inline mpack_node_t* mpack_node_map_at(mpack_node_t* node, size_t index, size_t offset) {
    if (mpack_node_error(node) != mpack_ok)
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

/**
 * Returns the key node in the given map at the given index.
 *
 * A nil node is returned in case of error.
 *
 * @throws mpack_error_type if the node is not a map
 * @throws mpack_error_data if the given index is out of bounds
 */
static inline mpack_node_t* mpack_node_map_key_at(mpack_node_t* node, size_t index) {
    return mpack_node_map_at(node, index, 0);
}

/**
 * Returns the value node in the given map at the given index.
 *
 * A nil node is returned in case of error.
 *
 * @throws mpack_error_type if the node is not a map
 * @throws mpack_error_data if the given index is out of bounds
 */
static inline mpack_node_t* mpack_node_map_value_at(mpack_node_t* node, size_t index) {
    return mpack_node_map_at(node, index, 1);
}

/**
 * Returns the value node in the given map for the given integer key. If the given
 * node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
static inline mpack_node_t* mpack_node_map_int(mpack_node_t* node, int64_t num) {
    return mpack_node_map_int_impl(node, num, false);
}

/**
 * Returns the value node in the given map for the given integer key, or NULL
 * if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
static inline mpack_node_t* mpack_node_map_int_optional(mpack_node_t* node, int64_t num) {
    return mpack_node_map_int_impl(node, num, true);
}

/**
 * Returns the value node in the given map for the given unsigned integer key. If
 * the given node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
static inline mpack_node_t* mpack_node_map_uint(mpack_node_t* node, uint64_t num) {
    return mpack_node_map_uint_impl(node, num, false);
}

/**
 * Returns the value node in the given map for the given unsigned integer
 * key, or NULL if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
static inline mpack_node_t* mpack_node_map_uint_optional(mpack_node_t* node, uint64_t num) {
    return mpack_node_map_uint_impl(node, num, true);
}

/**
 * Returns the value node in the given map for the given string key. If the given
 * node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
static inline mpack_node_t* mpack_node_map_str(mpack_node_t* node, const char* str, size_t length) {
    return mpack_node_map_str_impl(node, str, length, false);
}

/**
 * Returns the value node in the given map for the given string key, or NULL
 * if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
static inline mpack_node_t* mpack_node_map_str_optional(mpack_node_t* node, const char* str, size_t length) {
    return mpack_node_map_str_impl(node, str, length, true);
}

/**
 * Returns the value node in the given map for the given null-terminated string key.
 * If the given node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
static inline mpack_node_t* mpack_node_map_cstr(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_str(node, cstr, mpack_strlen(cstr));
}

/**
 * Returns the value node in the given map for the given null-terminated
 * string key, or NULL if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
static inline mpack_node_t* mpack_node_map_cstr_optional(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_str_optional(node, cstr, mpack_strlen(cstr));
}

/**
 * Returns true if the given node map contains a value for the given string key.
 * If the given node is not a map, mpack_error_type is raised and null is
 * returned.
 */
bool mpack_node_map_contains_str(mpack_node_t* node, const char* str, size_t length);

/**
 * Returns true if the given node map contains a value for the given
 * null-terminated string key. If the given node is not a map, mpack_error_type
 * is raised and null is returned.
 */
static inline bool mpack_node_map_contains_cstr(mpack_node_t* node, const char* cstr) {
    return mpack_node_map_contains_str(node, cstr, mpack_strlen(cstr));
}

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
#endif


