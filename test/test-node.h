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

#ifndef MPACK_TEST_NODE_H
#define MPACK_TEST_NODE_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_NODE

#define test_tree_destroy_noerror(tree) do { \
    test_assert(mpack_tree_error(tree) == mpack_ok, \
            "tree is in error state %i", (int)mpack_tree_error(tree)); \
    test_check_no_assertion(); \
    mpack_tree_destroy(tree); \
} while (0)

#define test_tree_destroy_error(tree, error) do { \
    mpack_error_t e = (error); \
    test_assert(mpack_tree_error(tree) == e, \
            "tree is in error state %i instead of %i", \
            (int)mpack_tree_error(tree), (int)e); \
    mpack_tree_destroy(tree); \
} while (0)

#define test_simple_tree_read(data, read_expr) do { \
  mpack_tree_t tree; \
  mpack_tree_init(&tree, data, sizeof(data) - 1); \
  mpack_node_t* node = mpack_tree_root(&tree); \
  test_check_no_assertion(); \
  test_assert((read_expr), "simple tree test did not pass: " #read_expr); \
  test_tree_destroy_noerror(&tree); \
} while (0)

#define test_simple_tree_read_error(data, read_expr, error) do { \
  mpack_tree_t tree; \
  mpack_tree_init(&tree, data, sizeof(data) - 1); \
  mpack_node_t* node = mpack_tree_root(&tree); \
  test_assert((read_expr), "simple read error test did not pass: " #read_expr); \
  test_tree_destroy_error(&tree, (error)); \
} while (0)

void test_node(void);

#endif

#ifdef __cplusplus
}
#endif

#endif


