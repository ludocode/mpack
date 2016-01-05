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

#ifndef MPACK_TEST_SYSTEM_H
#define MPACK_TEST_SYSTEM_H 1

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// Causes the next `count` system calls to succeed, and the
// following call to fail. If all is true, all subsequent
// calls will fail until the system is reset; otherwise the
// system is reset immediately.
void test_system_fail_after(size_t count, bool all);

// Resets the system call failure simulation, allowing all
// system calls to succeed
void test_system_fail_reset(void);

// Runs the given test repeatedly. On each iteration n, the test
// is run failing only the nth system call, and again failing the
// nth and all subsequent system calls. Repeats until both tests
// return true.
void test_system_fail_until_ok(bool (*test)(void));


// runs system tests
void test_system(void);


#ifdef MPACK_MALLOC
void* test_malloc(size_t size);
void* test_realloc(void* p, size_t size);
void test_free(void* p);

// Returns the number of mallocs that have not yet been freed.
size_t test_malloc_active_count(void);

// Returns the total number of mallocs or non-zero reallocs ever made.
size_t test_malloc_total_count(void);
#endif


#if defined(MPACK_STDIO) && MPACK_STDIO
FILE* test_fopen(const char* path, const char* mode);
int test_fclose(FILE* stream);
size_t test_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t test_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int test_fseek(FILE* stream, long offset, int whence);
long test_ftell(FILE* stream);
int test_ferror(FILE* stream);

// Returns the number of files that have not yet been closed.
size_t test_files_count(void);
#endif


#ifdef __cplusplus
}
#endif

#endif

