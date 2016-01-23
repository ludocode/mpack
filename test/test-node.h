/*
 * Copyright (c) 2015-2016 Nicholas Fraser
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

#ifndef MPACK_TEST_NODE_H
#define MPACK_TEST_NODE_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_NODE

extern mpack_error_t test_tree_error;
void test_tree_error_handler(mpack_tree_t* tree, mpack_error_t error);


#define TEST_TREE_DESTROY_NOERROR(tree) do { \
    mpack_error_t error = mpack_tree_destroy(tree); \
    TEST_TRUE(error == mpack_ok, "tree is in error state %i (%s)", \
            (int)error, mpack_error_to_string(error)); \
} while (0)

#define TEST_TREE_DESTROY_ERROR(tree, error) do { \
    mpack_error_t expected = (error); \
    mpack_error_t actual = mpack_tree_destroy(tree); \
    TEST_TRUE(actual == expected, "tree is in error state %i (%s) instead of %i (%s)", \
            (int)actual, mpack_error_to_string(actual), \
            (int)expected, mpack_error_to_string(expected)); \
} while (0)

#define TEST_SIMPLE_TREE_READ(data, read_expr) do { \
    mpack_tree_t tree; \
    mpack_tree_init_pool(&tree, data, sizeof(data) - 1, pool, sizeof(pool) / sizeof(*pool)); \
    mpack_tree_set_error_handler(&tree, test_tree_error_handler); \
    mpack_node_t node = mpack_tree_root(&tree); \
    TEST_TRUE((read_expr), "simple tree test did not pass: " #read_expr); \
    TEST_TREE_DESTROY_NOERROR(&tree); \
    TEST_TRUE(test_tree_error == mpack_ok); \
    test_tree_error = mpack_ok; \
} while (0)

#ifdef MPACK_MALLOC
#define TEST_TREE_INIT mpack_tree_init
#else
#define TEST_TREE_INIT(tree, data, data_size) \
    mpack_node_data_t pool[128]; \
mpack_tree_init_pool((tree), (data), (data_size), pool, sizeof(pool) / sizeof(*pool));
#endif

// the error handler is only called if the tree is not already in an
// error state, so we call it ourselves if the tree init failed.
#define TEST_SIMPLE_TREE_READ_ERROR(data, read_expr, error) do { \
    mpack_tree_t tree; \
    mpack_tree_init_pool(&tree, data, sizeof(data) - 1, pool, sizeof(pool) / sizeof(*pool)); \
    if (mpack_tree_error(&tree) != mpack_ok) \
        test_tree_error_handler(&tree, error); \
    mpack_tree_set_error_handler(&tree, test_tree_error_handler); \
    mpack_node_t node = mpack_tree_root(&tree); \
    TEST_TRUE((read_expr), "simple read error test did not pass: " #read_expr); \
    TEST_TREE_DESTROY_ERROR(&tree, (error)); \
    TEST_TRUE(test_tree_error == (error)); \
    test_tree_error = mpack_ok; \
} while (0)



// bug tests

#if MPACK_DEBUG

// runs a simple tree test, ensuring it causes an assert.
// we flag mpack_error_data to cancel out of any tracking.
// (note about volatile, see TEST_ASSERT())
#define TEST_SIMPLE_TREE_READ_ASSERT(data, read_expr) do { \
    volatile mpack_tree_t v_tree; \
    mpack_tree_t* tree = (mpack_tree_t*)(uintptr_t)&v_tree; \
    mpack_tree_init_pool(tree, data, sizeof(data) - 1, pool, sizeof(pool) / sizeof(*pool)); \
    mpack_node_t node = mpack_tree_root(tree); \
    TEST_ASSERT(read_expr); \
    mpack_tree_destroy(tree); \
} while (0)

#else

// we cannot test asserts in release mode because they are
// compiled away; code would continue to run and cause
// undefined behavior.
#define TEST_SIMPLE_TREE_READ_ASSERT(data, read_expr) ((void)0)

#endif

// runs a simple tree test, ensuring it causes a break in
// debug mode and flags mpack_error_bug in both debug and release.
#define TEST_SIMPLE_TREE_READ_BREAK(data, read_expr) do { \
    mpack_tree_t tree; \
    mpack_tree_init_pool(&tree, data, sizeof(data) - 1, pool, sizeof(pool) / sizeof(*pool)); \
    mpack_node_t node = mpack_tree_root(&tree); \
    TEST_BREAK(read_expr); \
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_bug); \
} while (0)



void test_node(void);

#endif

#ifdef __cplusplus
}
#endif

#endif


