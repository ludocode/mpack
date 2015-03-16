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
#error MPACK_NODE requires MPACK_READER.
#endif

#if !defined(MPACK_MALLOC) || !defined(MPACK_FREE)
#error MPACK_NODE requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE.
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
        struct mpack_node_t* children;
    } data;
};

/**
 * An MPack tree parsed from a blob of MessagePack.
 *
 * The tree contains a single root node which contains all parsed data.
 * The tree and its nodes are immutable.
 */
typedef struct mpack_tree_t mpack_tree_t;

struct mpack_tree_t {
    mpack_node_t root;
    mpack_node_t nil_node; // a nil node to be returned in case of error
    mpack_error_t error;
    size_t size;

    #if MPACK_SETJMP
    bool jump;          /* Whether to longjmp on error */
    jmp_buf jump_env;   /* Where to jump */
    #endif
};

/**
 * @name Tree Functions
 * @{
 */

/**
 * Initializes a tree by parsing the given data buffer. The tree must be destroyed
 * with mpack_tree_destroy(), even if parsing fails.
 *
 * Any string or blob data types reference the original data, so the data
 * pointer must remain valid until after the tree is destroyed.
 */
void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length);

/**
 * Returns the root node of the tree.
 */
static inline mpack_node_t* mpack_tree_root(mpack_tree_t* tree) {
    return &tree->root;
}

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

#if MPACK_SETJMP

/**
 * @hideinitializer
 *
 * Registers a jump target in case of error.
 *
 * If the tree is in an error state, 1 is returned when called. Otherwise 0 is
 * returned when called, and when the first error occurs, control flow will jump
 * to the point where MPACK_TREE_SETJMP() was called, resuming as though it
 * returned 1. This ensures an error handling block runs exactly once in case of
 * error.
 *
 * The argument may be evaluated multiple times.
 *
 * @returns 0 if the tree is not in an error state; 1 if and when an error occurs.
 */
#define MPACK_TREE_SETJMP(tree) (((tree)->error == mpack_ok) ? \
    ((tree)->jump = true, setjmp((tree)->jump_env)) : 1)

/**
 * Clears a jump target. Subsequent tree reading errors will not cause a jump.
 */
static inline void mpack_tree_clearjmp(mpack_tree_t* tree) {
    tree->jump = false;
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
 * Returns the tag contained by the given node.
 */
static inline mpack_tag_t mpack_node_tag(mpack_node_t* node) {
    return node->tag;
}

#if MPACK_DEBUG && MPACK_STDIO && MPACK_SETJMP
/*! Converts a node to JSON and pretty-prints it to stdout. */
void mpack_node_print(mpack_node_t* node);
#endif


/**
 * @}
 */

/**
 * @name STDIO Helpers
 * @{
 */

#if MPACK_STDIO
/**
 * An MPack tree parsed from a file containing a MessagePack object.
 *
 * The file tree can be used similar to a normal mpack_tree_t. It will
 * automatically open and load the file, and release the data when
 * destroyed.
 */
typedef struct mpack_file_tree_t mpack_file_tree_t;

struct mpack_file_tree_t {
    mpack_tree_t tree;
    char* data;
};

/**
 * Initializes a tree by reading and parsing the given file. The file tree must be
 * destroyed with mpack_file_tree_destroy(), even if parsing fails.
 *
 * The file is opened, loaded fully into memory, and closed before this call
 * returns. It is not accessed again by this file tree.
 *
 * @param file_tree The file tree to initialize
 * @param filename The filename passed to fopen() to read the file
 * @param max_size The maximum size of file to load, or 0 for unlimited size.
 */
void mpack_file_tree_init(mpack_file_tree_t* file_tree, const char* filename, int max_size);

/**
 * Destroys the file tree.
 */
mpack_error_t mpack_file_tree_destroy(mpack_file_tree_t* file_tree);

/**
 * Returns the error state of the file tree.
 */
static inline mpack_error_t mpack_file_tree_error(mpack_file_tree_t* file_tree) {
    return file_tree->tree.error;
}

/**
 * Returns the root node of the file tree.
 */
static inline mpack_node_t* mpack_file_tree_root(mpack_file_tree_t* file_tree) {
    return &file_tree->tree.root;
}

#if MPACK_SETJMP

/**
 * @hideinitializer
 *
 * Registers a jump target in case of error.
 *
 * If the file tree is in an error state, 1 is returned when called. Otherwise 0 is
 * returned when called, and when the first error occurs, control flow will jump
 * to the point where MPACK_FILE_TREE_SETJMP() was called, resuming as though it
 * returned 1. This ensures an error handling block runs exactly once in case of
 * error.
 *
 * The argument may be evaluated multiple times.
 *
 * @returns 0 if the file tree is not in an error state; 1 if and when an error occurs.
 */
#define MPACK_FILE_TREE_SETJMP(file_tree) (((file_tree)->tree.error == mpack_ok) ? \
    ((file_tree)->tree.jump = true, setjmp((file_tree)->tree.jump_env)) : 1)

/**
 * Clears a jump target. Subsequent tree reading errors will not cause a jump.
 */
static inline void mpack_file_tree_clearjmp(mpack_file_tree_t* file_tree) {
    file_tree->tree.jump = false;
}
#endif
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
    return node->tag.type;
}

/**
 * Checks if the given node is of nil type, raising mpack_error_type otherwise.
 */
void mpack_node_nil(mpack_node_t* node);

/**
 * Returns the bool value of the node. If this node is not of the correct
 * type, mpack_error_type is raised, and the return value should be discarded.
 */
bool mpack_node_bool(mpack_node_t* node);

/**
 * Checks if the given node is of bool type with value true, raising
 * mpack_error_type otherwise.
 */
void mpack_node_true(mpack_node_t* node);

/**
 * Checks if the given node is of bool type with value false, raising
 * mpack_error_type otherwise.
 */
void mpack_node_false(mpack_node_t* node);

/**
 * Returns the 8-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
uint8_t mpack_node_u8(mpack_node_t* node);

/**
 * Returns the 8-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
int8_t mpack_node_i8(mpack_node_t* node);

/**
 * Returns the 16-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
uint16_t mpack_node_u16(mpack_node_t* node);

/**
 * Returns the 16-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
int16_t mpack_node_i16(mpack_node_t* node);

/**
 * Returns the 32-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
uint32_t mpack_node_u32(mpack_node_t* node);

/**
 * Returns the 32-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
int32_t mpack_node_i32(mpack_node_t* node);

/**
 * Returns the 64-bit unsigned value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
uint64_t mpack_node_u64(mpack_node_t* node);

/**
 * Returns the 64-bit signed value of the node. If this node is not
 * of a compatible type, mpack_error_type is raised, and the
 * return value should be discarded.
 */
int64_t mpack_node_i64(mpack_node_t* node);

/**
 * Returns the float value of the node. The underlying value can be an
 * integer, float or double; the value is converted to a float.
 *
 * Note that reading a double or a large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
float mpack_node_float(mpack_node_t* node);

/**
 * Returns the double value of the node. The underlying value can be an
 * integer, float or double; the value is converted to a double.
 *
 * Note that reading a very large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
double mpack_node_double(mpack_node_t* node);

/**
 * Returns the float value of the node. The underlying value must be a float,
 * not a double or an integer. This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float.
 */
float mpack_node_float_strict(mpack_node_t* node);

/**
 * Returns the double value of the node. The underlying value must be a float
 * or double, not an integer. This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float or double.
 */
double mpack_node_double_strict(mpack_node_t* node);

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
int8_t mpack_node_exttype(mpack_node_t* node);

/**
 * Returns the length of the given str, bin or ext node.
 */
size_t mpack_node_data_len(mpack_node_t* node);

/**
 * Returns the length in bytes of the given string node. This does not
 * include any null-terminator.
 */
size_t mpack_node_strlen(mpack_node_t* node);

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
const char* mpack_node_data(mpack_node_t* node);

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

/**
 * @}
 */

/**
 * @name Compound Node Functions
 * @{
 */

/**
 * Returns the length of the given array node. Raises mpack_error_type
 * and returns 0 if the given node is not an array.
 */
size_t mpack_node_array_length(mpack_node_t* node);

/**
 * Returns the node in the given array at the given index. If the node
 * is not an array, mpack_error_type is raised and a nil node is returned.
 * If the given index is out of bounds, mpack_error_data is raised and
 * a nil node is returned.
 */
mpack_node_t* mpack_node_array_at(mpack_node_t* node, size_t index);

/**
 * Returns the number of key/value pairs in the given map node. Raises
 * mpack_error_type and returns 0 if the given node is not a map.
 */
size_t mpack_node_map_count(mpack_node_t* node);

/**
 * Returns the key node in the given map at the given index.
 *
 * A nil node is returned in case of error.
 *
 * @throws mpack_error_type if the node is not a map
 * @throws mpack_error_data if the given index is out of bounds
 */
mpack_node_t* mpack_node_map_key_at(mpack_node_t* node, size_t index);

/**
 * Returns the value node in the given map at the given index.
 *
 * A nil node is returned in case of error.
 *
 * @throws mpack_error_type if the node is not a map
 * @throws mpack_error_data if the given index is out of bounds
 */
mpack_node_t* mpack_node_map_value_at(mpack_node_t* node, size_t index);

/**
 * Returns the value node in the given map for the given integer key. If the given
 * node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
mpack_node_t* mpack_node_map_int(mpack_node_t* node, int64_t num);

/**
 * Returns the value node in the given map for the given integer key, or NULL
 * if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
mpack_node_t* mpack_node_map_int_option(mpack_node_t* node, int64_t num);

/**
 * Returns the value node in the given map for the given unsigned integer key. If
 * the given node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
mpack_node_t* mpack_node_map_uint(mpack_node_t* node, uint64_t num);

/**
 * Returns the value node in the given map for the given unsigned integer
 * key, or NULL if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
mpack_node_t* mpack_node_map_uint_option(mpack_node_t* node, uint64_t num);

/**
 * Returns the value node in the given map for the given string key. If the given
 * node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
mpack_node_t* mpack_node_map_str(mpack_node_t* node, const char* str, size_t length);

/**
 * Returns the value node in the given map for the given string key, or NULL
 * if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
mpack_node_t* mpack_node_map_str_option(mpack_node_t* node, const char* str, size_t length);

/**
 * Returns the value node in the given map for the given null-terminated string key.
 * If the given node is not a map, mpack_error_type is raised and a nil node is
 * returned. If the given key does not exist in the map, mpack_error_data
 * is raised and a nil node is returned.
 */
mpack_node_t* mpack_node_map_cstr(mpack_node_t* node, const char* cstr);

/**
 * Returns the value node in the given map for the given null-terminated
 * string key, or NULL if the map does not contain the given key.
 *
 * @throws mpack_error_type if the node is not a map
 */
mpack_node_t* mpack_node_map_cstr_option(mpack_node_t* node, const char* cstr);

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
bool mpack_node_map_contains_cstr(mpack_node_t* node, const char* cstr);

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


