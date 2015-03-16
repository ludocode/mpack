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

/*
 * test-malloc.h
 *
 * Implements a malloc that tracks allocs and frees to ensure they
 * match, and to count outstanding allocated blocks. It can also be
 * configured to fail to test correct out-of-memory handling.
 */

#ifndef MPACK_TEST_MALLOC_H
#define MPACK_TEST_MALLOC_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* test_malloc(size_t size);

// calls to test_malloc() will fail after count mallocs.
void test_malloc_fail_after(size_t count);

void test_malloc_reset(void);

void test_free(void* p);

// returns the number of mallocs that have not yet been freed.
size_t test_malloc_count(void);

#ifdef __cplusplus
}
#endif

#endif

