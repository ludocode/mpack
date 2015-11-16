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



/* For now, nothing in here should be seen by Doxygen. */
/** @cond */



#if defined(WIN32) && defined(MPACK_INTERNAL) && MPACK_INTERNAL
#define _CRT_SECURE_NO_WARNINGS 1
#endif



#include "mpack-config.h"

/*
 * Now that the config is included, we define to 0 any of the configuration
 * options and other switches that aren't defined. This supports -Wundef
 * without us having to write "#if defined(X) && X" everywhere (and while
 * allowing configs to be pre-defined to 0.)
 */
#ifndef MPACK_READER
#define MPACK_READER 0
#endif
#ifndef MPACK_EXPECT
#define MPACK_EXPECT 0
#endif
#ifndef MPACK_NODE
#define MPACK_NODE 0
#endif
#ifndef MPACK_WRITER
#define MPACK_WRITER 0
#endif

#ifndef MPACK_STDLIB
#define MPACK_STDLIB 0
#endif
#ifndef MPACK_STDIO
#define MPACK_STDIO 0
#endif

#ifndef MPACK_DEBUG
#define MPACK_DEBUG 0
#endif
#ifndef MPACK_CUSTOM_ASSERT
#define MPACK_CUSTOM_ASSERT 0
#endif
#ifndef MPACK_CUSTOM_BREAK
#define MPACK_CUSTOM_BREAK 0
#endif

#ifndef MPACK_READ_TRACKING
#define MPACK_READ_TRACKING 0
#endif
#ifndef MPACK_WRITE_TRACKING
#define MPACK_WRITE_TRACKING 0
#endif
#ifndef MPACK_NO_TRACKING
#define MPACK_NO_TRACKING 0
#endif
#ifndef MPACK_OPTIMIZE_FOR_SIZE
#define MPACK_OPTIMIZE_FOR_SIZE 0
#endif

#ifndef MPACK_EMIT_INLINE_DEFS
#define MPACK_EMIT_INLINE_DEFS 0
#endif
#ifndef MPACK_AMALGAMATED
#define MPACK_AMALGAMATED 0
#endif
#ifndef MPACK_RELEASE_VERSION
#define MPACK_RELEASE_VERSION 0
#endif
#ifndef MPACK_INTERNAL
#define MPACK_INTERNAL 0
#endif



/* System headers (based on configuration) */

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
#include <errno.h>
#endif



/*
 * Header configuration
 */

#ifdef __cplusplus
#define MPACK_EXTERN_C_START extern "C" {
#define MPACK_EXTERN_C_END   }
#else
#define MPACK_EXTERN_C_START /* nothing */
#define MPACK_EXTERN_C_END   /* nothing */
#endif

/* GCC versions from 4.6 to before 5.1 warn about defining a C99
 * non-static inline function before declaring it (see issue #20) */
#ifdef __GNUC__
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#ifdef __cplusplus
#define MPACK_DECLARED_INLINE_WARNING_START \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wmissing-declarations\"")
#else
#define MPACK_DECLARED_INLINE_WARNING_START \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wmissing-prototypes\"")
#endif
#define MPACK_DECLARED_INLINE_WARNING_END \
    _Pragma ("GCC diagnostic pop")
#endif
#endif
#ifndef MPACK_DECLARED_INLINE_WARNING_START
#define MPACK_DECLARED_INLINE_WARNING_START /* nothing */
#define MPACK_DECLARED_INLINE_WARNING_END /* nothing */
#endif

/* GCC versions before 4.8 warn about shadowing a function with a
 * variable that isn't a function or function pointer (like "index") */
#ifdef __GNUC__
#if (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#define MPACK_WSHADOW_WARNING_START \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wshadow\"")
#define MPACK_WSHADOW_WARNING_END \
    _Pragma ("GCC diagnostic pop")
#endif
#endif
#ifndef MPACK_WSHADOW_WARNING_START
#define MPACK_WSHADOW_WARNING_START /* nothing */
#define MPACK_WSHADOW_WARNING_END /* nothing */
#endif

#define MPACK_HEADER_START \
    MPACK_EXTERN_C_START \
    MPACK_WSHADOW_WARNING_START \
    MPACK_DECLARED_INLINE_WARNING_START

#define MPACK_HEADER_END \
    MPACK_DECLARED_INLINE_WARNING_END \
    MPACK_WSHADOW_WARNING_END \
    MPACK_EXTERN_C_END

MPACK_HEADER_START



/* Miscellaneous helper macros */

#define MPACK_UNUSED(var) ((void)(var))

#define MPACK_STRINGIFY_IMPL(arg) #arg
#define MPACK_STRINGIFY(arg) MPACK_STRINGIFY_IMPL(arg)



/*
 * Definition of inline macros.
 *
 * MPack supports several different modes for inline functions:
 *   - functions declared with a platform-specific always-inline (MPACK_ALWAYS_INLINE)
 *   - functions declared inline regardless of optimization options (MPACK_INLINE)
 *   - functions declared inline only in builds optimized for speed (MPACK_INLINE_SPEED)
 *
 * MPack does not use static inline in header files; only one non-inline definition
 * of each function should exist in the final build. This can reduce the binary size
 * in cases where the compiler cannot or chooses not to inline a function.
 * The addresses of functions should also compare equal across translation units
 * regardless of whether they are declared inline.
 *
 * The above requirements mean that the declaration and definition of non-trivial
 * inline functions must be separated so that the definitions will only
 * appear when necessary. In addition, three different linkage models need
 * to be supported:
 *
 *  - The C99 model, where a standalone function is emitted only if there is any
 *    `extern inline` or non-`inline` declaration (including the definition itself)
 *
 *  - The GNU model, where an `inline` definition emits a standalone function and an
 *    `extern inline` definition does not, regardless of other declarations
 *
 *  - The C++ model, where `inline` emits a standalone function with special
 *    (COMDAT) linkage
 *
 * The macros below wrap up everything above. All inline functions defined in header
 * files have a single non-inline definition emitted in the compilation of
 * mpack-platform.c. All inline declarations and definitions use the same MPACK_INLINE
 * specification to simplify the rules on when standalone functions are emitted.
 *
 * Inline functions in source files are defined static, so MPACK_STATIC_INLINE
 * is used for small functions and MPACK_STATIC_INLINE_SPEED is used for
 * larger optionally inline functions.
 *
 * Additional reading:
 *     http://www.greenend.org.uk/rjk/tech/inline.html
 */

#if defined(__cplusplus)
    // C++ rules
    // The linker will need COMDAT support to link C++ object files,
    // so we don't need to worry about emitting definitions from C++
    // translation units. If mpack-platform.c (or the amalgamation)
    // is compiled as C, its definition will be used, otherwise a
    // C++ definition will be used, and no other C files will emit
    // a defition.
    #define MPACK_INLINE inline
#elif defined(__GNUC__) && (defined(__GNUC_GNU_INLINE__) || \
        !defined(__GNUC_STDC_INLINE__) && !defined(__GNUC_GNU_INLINE__))
    // GNU rules
    #if MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE inline
    #else
        #define MPACK_INLINE extern inline
    #endif
#else
    // C99 rules
    #if MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE extern inline
    #else
        #define MPACK_INLINE inline
    #endif
#endif

#define MPACK_STATIC_INLINE static inline

#if MPACK_OPTIMIZE_FOR_SIZE
    #define MPACK_STATIC_INLINE_SPEED static
    #define MPACK_INLINE_SPEED /* nothing */
    #if MPACK_EMIT_INLINE_DEFS
        #define MPACK_DEFINE_INLINE_SPEED 1
    #else
        #define MPACK_DEFINE_INLINE_SPEED 0
    #endif
#else
    #define MPACK_STATIC_INLINE_SPEED static inline
    #define MPACK_INLINE_SPEED MPACK_INLINE
    #define MPACK_DEFINE_INLINE_SPEED 1
#endif

#ifdef MPACK_OPTIMIZE_FOR_SPEED
#error "You should define MPACK_OPTIMIZE_FOR_SIZE, not MPACK_OPTIMIZE_FOR_SPEED."
#endif



/* Some compiler-specific keywords and builtins */

#if defined(__GNUC__) || defined(__clang__)
    #define MPACK_UNREACHABLE __builtin_unreachable()
    #define MPACK_NORETURN(fn) fn __attribute__((noreturn))

    // gcov gets confused with always_inline, so we disable it under the unit tests
    #ifndef MPACK_GCOV
        #define MPACK_ALWAYS_INLINE __attribute__((always_inline)) MPACK_INLINE
        #define MPACK_STATIC_ALWAYS_INLINE static __attribute__((always_inline)) inline
    #endif

#elif defined(_MSC_VER)
    #define MPACK_UNREACHABLE __assume(0)
    #define MPACK_NORETURN(fn) __declspec(noreturn) fn
    #define MPACK_ALWAYS_INLINE __forceinline
    #define MPACK_STATIC_ALWAYS_INLINE static __forceinline
#endif

#ifndef MPACK_UNREACHABLE
    #define MPACK_UNREACHABLE ((void)0)
#endif
#ifndef MPACK_NORETURN
    #define MPACK_NORETURN(fn) fn
#endif
#ifndef MPACK_ALWAYS_INLINE
    #define MPACK_ALWAYS_INLINE MPACK_INLINE
#endif
#ifndef MPACK_STATIC_ALWAYS_INLINE
    #define MPACK_STATIC_ALWAYS_INLINE static inline
#endif



/* Static assert */

#ifndef MPACK_STATIC_ASSERT
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
#define MPACK_STATIC_ASSERT _Static_assert
#endif
#endif
#endif

#ifndef MPACK_STATIC_ASSERT
#if defined(__has_feature)
#if __has_feature(cxx_static_assert)
#define MPACK_STATIC_ASSERT static_assert
#elif __has_feature(c_static_assert)
#define MPACK_STATIC_ASSERT _Static_assert
#endif
#endif
#endif

#ifndef MPACK_STATIC_ASSERT
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define MPACK_STATIC_ASSERT static_assert
#endif
#endif
#endif

#ifndef MPACK_STATIC_ASSERT
#if defined(__GNUC__)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#ifndef __cplusplus
#define MPACK_STATIC_ASSERT(expr, str) do { \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-pedantic\"") \
    _Pragma ("GCC diagnostic ignored \"-Wc++-compat\"") \
    _Static_assert(expr, str); \
    _Pragma ("GCC diagnostic pop") \
} while (0)
#endif
#endif
#endif
#endif

#ifndef MPACK_STATIC_ASSERT
#ifdef _MSC_VER
#if _MSC_VER >= 1600
#define MPACK_STATIC_ASSERT static_assert
#endif
#endif
#endif

#ifndef MPACK_STATIC_ASSERT
#define MPACK_STATIC_ASSERT(expr, str) /* nothing */
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
 * the case of this being false. Writing mpack_assert(0) rarely makes sense (except
 * possibly as a default handler in a switch) since the compiler will throw away any
 * code after it. If at any time an mpack_assert() is not true, the behaviour is
 * undefined. This also means the expression is evaluated even in release.
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



/* Wrap some needed libc functions */
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
#if MPACK_READ_TRACKING && !defined(MPACK_READER)
    #error "MPACK_READ_TRACKING requires MPACK_READER."
#endif
#if MPACK_WRITE_TRACKING && !defined(MPACK_WRITER)
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
    MPACK_ALWAYS_INLINE void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size) {
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

MPACK_HEADER_END

/** @endcond */

#endif

