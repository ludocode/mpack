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
static const int test_system_fail_until_max = 500;

void test_system_fail_after(size_t count) {
    test_system_fail = true;
    test_system_left = count;
}

void test_system_fail_reset(void) {
    test_system_fail = false;
}

#if defined(MPACK_MALLOC) || MPACK_STDIO
static bool test_system_should_fail(void) {
    if (!test_system_fail)
        return false;
    if (test_system_left == 0)
        return true;
    --test_system_left;
    return false;
}
#endif

void test_system_fail_until_ok(bool (*test)(void)) {
    #ifdef MPACK_MALLOC
    TEST_TRUE(test_malloc_active_count() == 0, "allocations exist before starting failure test");
    #endif
    #if MPACK_STDIO
    TEST_TRUE(test_files_count() == 0, "files are open before starting failure test");
    #endif

    for (int i = 0; i < test_system_fail_until_max; ++i) {
        test_system_fail_after(i);
        bool ok = test();

        #ifdef MPACK_MALLOC
        TEST_TRUE(test_malloc_active_count() == 0, "test leaked memory on iteration %i!", i);
        #endif
        #if MPACK_STDIO
        TEST_TRUE(test_files_count() == 0, "test leaked file on iteration %i!", i);
        #endif

        if (ok) {
            test_system_fail_reset();
            return;
        }
    }

    TEST_TRUE(false, "hit maximum number of system calls in a system fail test");
    test_system_fail_reset();
}

void test_system(void) {
    #ifdef MPACK_MALLOC
    TEST_TRUE(test_malloc_active_count() == 0);
    TEST_TRUE(NULL == mpack_realloc(NULL, 0, 0));
    void* p = MPACK_MALLOC(1);
    TEST_TRUE(NULL == mpack_realloc(p, 1, 0));
    TEST_TRUE(test_malloc_active_count() == 0, "realloc leaked");
    #endif
}



#ifdef MPACK_MALLOC
static size_t test_malloc_active = 0;
static size_t test_malloc_total = 0;

size_t test_malloc_active_count(void) {
    return test_malloc_active;
}

size_t test_malloc_total_count(void) {
    return test_malloc_total;
}

void* test_malloc(size_t size) {
    TEST_TRUE(size != 0, "cannot allocate zero bytes!");
    if (size == 0)
        return NULL;

    if (test_system_should_fail())
        return NULL;

    ++test_malloc_total;
    ++test_malloc_active;
    return malloc(size);
}

void* test_realloc(void* p, size_t size) {
    if (size == 0) {
        if (p) {
            free(p);
            --test_malloc_active;
        }
        return NULL;
    }

    if (test_system_should_fail())
        return NULL;

    ++test_malloc_total;
    if (!p)
        ++test_malloc_active;
    return realloc(p, size);
}

void test_free(void* p) {
    // while free() is supposed to allow NULL, not all custom allocators
    // may handle this, so we don't free NULL.
    TEST_TRUE(p != NULL, "attempting to free NULL");

    if (p)
        --test_malloc_active;
    free(p);
}

#endif



#if MPACK_STDIO
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef ferror

static size_t test_files_active = 0;

size_t test_files_count(void) {
    return test_files_active;
}

FILE* test_fopen(const char* path, const char* mode) {
    if (test_system_should_fail()) {
        errno = EACCES;
        return NULL;
    }

    FILE* file = fopen(path, mode);
    if (file)
        ++test_files_active;
    return file;
}

int test_fclose(FILE* stream) {
    TEST_TRUE(stream != NULL);

    --test_files_active;

    // if we're simulating failure, we still close the file
    // anyway to avoid leaking any files
    int ret = fclose(stream);

    if (test_system_should_fail()) {
        errno = EACCES;
        return EOF;
    }

    return ret;
}

size_t test_fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    TEST_TRUE(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return 0;
    }

    return fread(ptr, size, nmemb, stream);
}

size_t test_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    TEST_TRUE(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return 0;
    }

    return fwrite(ptr, size, nmemb, stream);
}

int test_fseek(FILE* stream, long offset, int whence) {
    TEST_TRUE(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return -1;
    }

    return fseek(stream, offset, whence);
}

long test_ftell(FILE* stream) {
    TEST_TRUE(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return -1;
    }

    return ftell(stream);
}

int test_ferror(FILE * stream) {
    TEST_TRUE(stream != NULL);

    if (test_system_should_fail()) {
        errno = EACCES;
        return -1;
    }

    return ferror(stream);
}
#endif

