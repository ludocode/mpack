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

// We need to include test.h here instead of test-system.h because
// test-system.h is included within the mpack-config.h.
#include "test.h"

#if MPACK_STDIO
#include <errno.h>
#endif

static bool test_system_fail = false;
static size_t test_system_left = 0;

void test_system_fail_after(size_t count) {
    test_system_fail = true;
    test_system_left = count;
}

void test_system_fail_reset(void) {
    test_system_fail = false;
}

#if defined(MPACK_MALLOC) || (defined(MPACK_STDIO) && MPACK_STDIO)
static bool test_system_should_fail(void) {
    if (!test_system_fail)
        return false;
    if (test_system_left == 0)
        return true;
    --test_system_left;
    return false;
}
#endif

#ifdef MPACK_MALLOC
static size_t test_malloc_active = 0;

void* test_malloc(size_t size) {
    test_assert(size != 0, "cannot allocate zero bytes!");
    if (size == 0)
        return NULL;

    if (test_system_should_fail())
        return NULL;

    ++test_malloc_active;
    return malloc(size);
}

void* test_realloc(void* p, size_t size) {
    test_assert(size != 0, "cannot allocate zero bytes!");
    if (size == 0) {
        if (p) {
            free(p);
            --test_malloc_active;
        }
        return NULL;
    }

    if (test_system_should_fail())
        return NULL;

    if (!p)
        ++test_malloc_active;
    return realloc(p, size);
}

void test_free(void* p) {
    // while free() is supposed to allow NULL, not all custom allocators
    // may handle this, so we don't free NULL.
    test_assert(p != NULL, "attempting to free NULL");

    if (p)
        --test_malloc_active;
    free(p);
}

size_t test_malloc_count(void) {
    return test_malloc_active;
}

#endif

#if MPACK_STDIO
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell

FILE* test_fopen(const char* path, const char* mode) {
    if (test_system_should_fail()) {
        errno = EACCES;
        return NULL;
    }
    return fopen(path, mode);
}

int test_fclose(FILE* stream) {
    test_assert(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return EOF;
    }

    return fclose(stream);
}

size_t test_fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    test_assert(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return 0;
    }

    return fread(ptr, size, nmemb, stream);
}

size_t test_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    test_assert(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return 0;
    }

    return fwrite(ptr, size, nmemb, stream);
}

int test_fseek(FILE* stream, long offset, int whence) {
    test_assert(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return -1;
    }

    return fseek(stream, offset, whence);
}

long test_ftell(FILE* stream) {
    test_assert(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return -1;
    }

    return ftell(stream);
}
#endif

