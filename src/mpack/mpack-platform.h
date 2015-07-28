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
 * implementations of standard C functions when libc is not available,
 * as well as wrappers to library functions.
 */

#ifndef MPACK_PLATFORM_H
#define MPACK_PLATFORM_H 1

#if defined(WIN32) && MPACK_INTERNAL
#define _CRT_SECURE_NO_WARNINGS 1
#endif

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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

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

#if MPACK_AMALGAMATED
#define MPACK_INTERNAL_STATIC static
#else
#define MPACK_INTERNAL_STATIC
#endif

#define MPACK_STRINGIFY_IMPL(arg) #arg
#define MPACK_STRINGIFY(arg) MPACK_STRINGIFY_IMPL(arg)



/* Some compiler-specific keywords and builtins */
#if defined(__GNUC__) || defined(__clang__)
    #define MPACK_UNREACHABLE __builtin_unreachable()
    #define MPACK_NORETURN(fn) fn __attribute__((noreturn))
    #define MPACK_ALWAYS_INLINE __attribute__((always_inline)) static inline
#elif _MSC_VER
    #define MPACK_UNREACHABLE __assume(0)
    #define MPACK_NORETURN(fn) __declspec(noreturn) fn
    #define MPACK_ALWAYS_INLINE __forceinline static
#else
    #define MPACK_UNREACHABLE ((void)0)
    #define MPACK_NORETURN(fn) fn
    #define MPACK_ALWAYS_INLINE static inline
#endif



/*
 * Here we define mpack_assert() and mpack_break(). They both work like a normal
 * assertion function in debug mode, causing a trap or abort. However, on some platforms
 * you can safely resume execution from mpack_break(), whereas mpack_assert() is
 * always fatal.
 *
 * In release mode, mpack_assert() is converted to an assurance to the compiler
 * that the expression cannot be false (via e.g. __assume() or __builtin_unreachable())
 * to improve optimization where supported. There is thus no point in "safely" handling
 * the case of this being false. Writing mpack_assert(0) rarely makes sense;
 * the compiler will throw away any code after it. If at any time an mpack_assert()
 * is not true, the behaviour is undefined. This also means the expression is
 * evaluated even in release.
 *
 * mpack_break() on the other hand is compiled to nothing in release. It is
 * used in situations where we want to highlight a programming error as early as
 * possible (in the debugger), but we still handle the situation safely if it
 * happens in release to avoid producing incorrect results (such as in
 * MPACK_WRITE_TRACKING.) It does not take an expression to test because it
 * belongs in a safe-handling block after its failing condition has been tested.
 *
 * If stdio is available, we can add a format string describing the error, and
 * on some compilers we can declare it noreturn to get correct results from static
 * analysis tools. Note that the format string and arguments are not evaluated unless
 * the assertion is hit.
 *
 * Note that any arguments to mpack_assert() beyond the first are only evaluated
 * if the expression is false (and are never evaluated in release.)
 *
 * mpack_assert_fail() and mpack_break_hit() are defined separately
 * because assert is noreturn and break isn't. This distinction is very
 * important for static analysis tools to give correct results.
 */

#if MPACK_DEBUG
    MPACK_NORETURN(void mpack_assert_fail(const char* message));
    #if MPACK_STDIO
        MPACK_NORETURN(void mpack_assert_fail_format(const char* format, ...));
        #define mpack_assert_fail_at(line, file, expr, ...) \
                mpack_assert_fail_format("mpack assertion failed at " file ":" #line "\n" expr "\n" __VA_ARGS__)
    #else
        #define mpack_assert_fail_at(line, file, ...) \
                mpack_assert_fail("mpack assertion failed at " file ":" #line )
    #endif

    #define mpack_assert_fail_pos(line, file, expr, ...) mpack_assert_fail_at(line, file, expr, __VA_ARGS__)
    #define mpack_assert(expr, ...) ((!(expr)) ? mpack_assert_fail_pos(__LINE__, __FILE__, #expr, __VA_ARGS__) : (void)0)

    void mpack_break_hit(const char* message);
    #if MPACK_STDIO
        void mpack_break_hit_format(const char* format, ...);
        #define mpack_break_hit_at(line, file, ...) \
                mpack_break_hit_format("mpack breakpoint hit at " file ":" #line "\n" __VA_ARGS__)
    #else
        #define mpack_break_hit_at(line, file, ...) \
                mpack_break_hit("mpack breakpoint hit at " file ":" #line )
    #endif
    #define mpack_break_hit_pos(line, file, ...) mpack_break_hit_at(line, file, __VA_ARGS__)
    #define mpack_break(...) mpack_break_hit_pos(__LINE__, __FILE__, __VA_ARGS__)
#else
    #define mpack_assert(expr, ...) ((!(expr)) ? MPACK_UNREACHABLE, (void)0 : (void)0)
    #define mpack_break(...) ((void)0)
#endif



#if MPACK_STDLIB
#define mpack_memset memset
#define mpack_memcpy memcpy
#define mpack_memmove memmove
#define mpack_memcmp memcmp
#define mpack_strlen strlen
#else
void* mpack_memset(void *s, int c, size_t n);
void* mpack_memcpy(void *s1, const void *s2, size_t n);
void* mpack_memmove(void *s1, const void *s2, size_t n);
int mpack_memcmp(const void* s1, const void* s2, size_t n);
size_t mpack_strlen(const char *s);
#endif



/* Debug logging */
#if 0
#define mpack_log(...) printf(__VA_ARGS__);
#else
#define mpack_log(...) ((void)0)
#endif



/* Make sure our configuration makes sense */
#if defined(MPACK_MALLOC) && !defined(MPACK_FREE)
    #error "MPACK_MALLOC requires MPACK_FREE."
#endif
#if !defined(MPACK_MALLOC) && defined(MPACK_FREE)
    #error "MPACK_FREE requires MPACK_MALLOC."
#endif
#if MPACK_READ_TRACKING && (!defined(MPACK_READER) || !MPACK_READER)
    #error "MPACK_READ_TRACKING requires MPACK_READER."
#endif
#if MPACK_WRITE_TRACKING && (!defined(MPACK_WRITER) || !MPACK_WRITER)
    #error "MPACK_WRITE_TRACKING requires MPACK_WRITER."
#endif
#ifndef MPACK_MALLOC
    #if MPACK_STDIO
    #error "MPACK_STDIO requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #if MPACK_READ_TRACKING
    #error "MPACK_READ_TRACKING requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #if MPACK_WRITE_TRACKING
    #error "MPACK_WRITE_TRACKING requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
#endif



/* Implement realloc if unavailable */
#ifdef MPACK_MALLOC
    #ifdef MPACK_REALLOC
    static inline void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size) {
        MPACK_UNUSED(used_size);
        return MPACK_REALLOC(old_ptr, new_size);
    }
    #else
    void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size);
    #endif
#endif



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

