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

// We need to include test.h here instead of test-malloc.h because
// otherwise MPACK_MALLOC will not be defined yet.
#include "test.h"

#ifdef MPACK_MALLOC

static bool test_malloc_fail = false;
static size_t test_malloc_left = 0;
static size_t test_malloc_active = 0;

void* test_malloc(size_t size) {
    if (test_malloc_fail) {
        if (test_malloc_left == 0)
            return NULL;
        --test_malloc_left;
    }
    ++test_malloc_active;
    return malloc(size);
}

void* test_realloc(void* p, size_t size) {
    if (test_malloc_fail) {
        if (test_malloc_left == 0)
            return NULL;
        --test_malloc_left;
    }
    if (!p)
        ++test_malloc_active;
    return realloc(p, size);
}

void test_free(void* p) {
    // while free() is supposed to allow NULL, not all custom allocators
    // may handle this, so we don't free NULL.
    test_assert(p != NULL, "attempting to free NULL");

    --test_malloc_active;
    free(p);
}

void test_malloc_fail_after(size_t count) {
    test_malloc_fail = true;
    test_malloc_left = count;
}

void test_malloc_reset(void) {
    test_malloc_fail = false;
}

size_t test_malloc_count(void) {
    return test_malloc_active;
}

#endif

