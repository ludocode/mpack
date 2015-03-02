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
 * Abstracts all platform-specific code from MPack. This contains
 * implementations of standard C functions when libc is not available.
 */

#ifndef MPACK_PLATFORM_H
#define MPACK_PLATFORM_H 1

#include "mpack-config.h"

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1
#endif

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#if MPACK_STDLIB
#include <string.h>
#include <stdlib.h>
#endif
#if MPACK_STDIO
#include <stdio.h>
#endif
#if MPACK_SETJMP
#include <setjmp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MPACK_UNUSED(var) ((void)(var))

/* Define mpack_assert() depending on configuration. If stdio is */
/* available, we can add a format string describing the error. */
#if MPACK_DEBUG
    void mpack_assert_fail(const char* message);
    #if MPACK_STDIO
        void mpack_assert_fail_format(const char* format, ...);
        #define mpack_assert_fail_at(line, file, expr, ...) \
                mpack_assert_fail_format("mpack assertion failed at " file ":" #line "\n" expr "\n" __VA_ARGS__)
    #else
        #define mpack_assert_fail_at(line, file, expr, ...) \
                mpack_assert_fail("mpack assertion failed at " file ":" #line "\n" expr)
    #endif
    #define mpack_assert_fail_pos(line, file, expr, ...) mpack_assert_fail_at(line, file, expr, __VA_ARGS__)
    #define mpack_assert(expr, ...) ((expr) ? (void)0 : mpack_assert_fail_pos(__LINE__, __FILE__, #expr, __VA_ARGS__))
#else
    #define mpack_assert(expr, ...) ((void)0)
#endif


#if MPACK_STDLIB
#define mpack_memcmp memcmp
#define mpack_strlen strlen
#else
int mpack_memcmp(const void* s1, const void* s2, size_t n);
size_t mpack_strlen(const char *s);
#endif

/* Clean up the debug logging */
#define mpack_log(...) ((void)0)
/* #define mpack_log(...) printf(__VA_ARGS__); */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

