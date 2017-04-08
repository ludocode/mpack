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

/**
 * @file
 *
 * Abstracts all platform-specific code from MPack. This contains
 * implementations of standard C functions when libc is not available,
 * as well as wrappers to library functions.
 */

#ifndef MPACK_PLATFORM_H
#define MPACK_PLATFORM_H 1



/* Pre-include checks */

#if defined(_MSC_VER) && _MSC_VER < 1800 && !defined(__cplusplus)
#error "In Visual Studio 2012 and earlier, MPack must be compiled as C++. Enable the /Tp compiler flag."
#endif

#if defined(WIN32) && defined(MPACK_INTERNAL) && MPACK_INTERNAL
#define _CRT_SECURE_NO_WARNINGS 1
#endif



/* Include the custom config file if enabled */

#ifdef MPACK_DOXYGEN
#define MPACK_HAS_CONFIG 1
#elif defined(MPACK_HAS_CONFIG) && MPACK_HAS_CONFIG
#include "mpack-config.h"
#endif

/*
 * Now that the optional config is included, we define the defaults
 * for any of the configuration options and other switches that aren't
 * yet defined.
 *
 * In development mode, or when the library is used without being
 * amalgamated, we set the defaults by including the sample config
 * directly. When amalgamated, this is disabled and the contents
 * of the sample config have been inserted into the top of mpack.h.
 */
#include "mpack-config.h.sample"

/*
 * All remaining configuration options that have not yet been set must
 * be defined here in order to support -Wundef.
 */
#ifndef MPACK_DEBUG
#define MPACK_DEBUG 0
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
#ifndef MPACK_NO_BUILTINS
#define MPACK_NO_BUILTINS 0
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
 * Inline functions in source files are defined MPACK_STATIC_INLINE.
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

#elif defined(_MSC_VER)
    // MSVC 2013 always uses COMDAT linkage, but it doesn't treat
    // 'inline' as a keyword in C99 mode.
    #define MPACK_INLINE __inline
    #define MPACK_STATIC_INLINE static __inline

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

#ifndef MPACK_STATIC_INLINE
#define MPACK_STATIC_INLINE static inline
#endif

#ifdef MPACK_OPTIMIZE_FOR_SPEED
    #error "You should define MPACK_OPTIMIZE_FOR_SIZE, not MPACK_OPTIMIZE_FOR_SPEED."
#endif



/*
 * Prevent inlining
 *
 * When a function is only used once, it is almost always inlined
 * automatically. This can cause poor instruction cache usage because a
 * function that should rarely be called (such as buffer exhaustion handling)
 * will get inlined into the middle of a hot code path.
 */

#if !MPACK_NO_BUILTINS
    #if defined(_MSC_VER)
        #define MPACK_NOINLINE __declspec(noinline)
    #elif defined(__GNUC__) || defined(__clang__)
        #define MPACK_NOINLINE __attribute__((noinline))
    #endif
#endif
#ifndef MPACK_NOINLINE
    #define MPACK_NOINLINE /* nothing */
#endif



/* Some compiler-specific keywords and builtins */

#if !MPACK_NO_BUILTINS
    #if defined(__GNUC__) || defined(__clang__)
        #define MPACK_UNREACHABLE __builtin_unreachable()
        #define MPACK_NORETURN(fn) fn __attribute__((noreturn))
        #define MPACK_RESTRICT __restrict__
    #elif defined(_MSC_VER)
        #define MPACK_UNREACHABLE __assume(0)
        #define MPACK_NORETURN(fn) __declspec(noreturn) fn
        #define MPACK_RESTRICT __restrict
    #endif
#endif

#ifndef MPACK_RESTRICT
#ifdef __cplusplus
#define MPACK_RESTRICT /* nothing, unavailable in C++ */
#else
#define MPACK_RESTRICT restrict /* required in C99 */
#endif
#endif

#ifndef MPACK_UNREACHABLE
#define MPACK_UNREACHABLE ((void)0)
#endif
#ifndef MPACK_NORETURN
#define MPACK_NORETURN(fn) fn
#endif



/*
 * Likely/unlikely
 *
 * These should only really be used when a branch is taken (or not taken) less
 * than 1/1000th of the time. Buffer flush checks when writing very small
 * elements are a good example.
 */

#if !MPACK_NO_BUILTINS
    #if defined(__GNUC__) || defined(__clang__)
        #ifndef MPACK_LIKELY
            #define MPACK_LIKELY(x) __builtin_expect((x),1)
        #endif
        #ifndef MPACK_UNLIKELY
            #define MPACK_UNLIKELY(x) __builtin_expect((x),0)
        #endif
    #endif
#endif

#ifndef MPACK_LIKELY
    #define MPACK_LIKELY(x) (x)
#endif
#ifndef MPACK_UNLIKELY
    #define MPACK_UNLIKELY(x) (x)
#endif



/* Static assert */

#ifndef MPACK_STATIC_ASSERT
    #if defined(__cplusplus)
        #if __cplusplus >= 201103L
            #define MPACK_STATIC_ASSERT static_assert
        #endif
    #elif defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #define MPACK_STATIC_ASSERT _Static_assert
        #endif
    #endif
#endif

#if !MPACK_NO_BUILTINS
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
        #if defined(__GNUC__)
            #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
                #ifndef __cplusplus
                    #if __GNUC__ >= 5
                    #define MPACK_IGNORE_PEDANTIC "GCC diagnostic ignored \"-Wpedantic\""
                    #else
                    #define MPACK_IGNORE_PEDANTIC "GCC diagnostic ignored \"-pedantic\""
                    #endif
                    #define MPACK_STATIC_ASSERT(expr, str) do { \
                        _Pragma ("GCC diagnostic push") \
                        _Pragma (MPACK_IGNORE_PEDANTIC) \
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
#endif

#ifndef MPACK_STATIC_ASSERT
    #define MPACK_STATIC_ASSERT(expr, str) (MPACK_UNUSED(sizeof(char[1 - 2*!(expr)])))
#endif



/* _Generic */

#ifndef MPACK_HAS_GENERIC
    #if defined(__clang__) && defined(__has_feature)
        // With Clang we can test for _Generic support directly
        // and ignore C/C++ version
        #if __has_feature(c_generic_selections)
            #define MPACK_HAS_GENERIC 1
        #else
            #define MPACK_HAS_GENERIC 0
        #endif
    #endif
#endif

#ifndef MPACK_HAS_GENERIC
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #if defined(__GNUC__) && !defined(__clang__)
                // GCC does not have full C11 support in GCC 4.7 and 4.8
                #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
                    #define MPACK_HAS_GENERIC 1
                #endif
            #else
                // We hope other compilers aren't lying about C11/_Generic support
                #define MPACK_HAS_GENERIC 1
            #endif
        #endif
    #endif
#endif

#ifndef MPACK_HAS_GENERIC
    #define MPACK_HAS_GENERIC 0
#endif



/*
 * Finite Math
 *
 * -ffinite-math-only, included in -ffast-math, breaks functions that
 * that check for non-finite real values such as isnan() and isinf().
 *
 * We should use this to trap errors when reading data that contains
 * non-finite reals. This isn't currently implemented.
 */

#ifndef MPACK_FINITE_MATH
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#define MPACK_FINITE_MATH 1
#endif
#endif

#ifndef MPACK_FINITE_MATH
#define MPACK_FINITE_MATH 0
#endif



/*
 * Endianness checks
 *
 * These define MPACK_NHSWAP*() which swap network<->host byte
 * order when needed.
 *
 * We leave them undefined if we can't determine the endianness
 * at compile-time, in which case we fall back to bit-shifts.
 *
 * See the notes in mpack-common.h.
 */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define MPACK_NHSWAP16(x) (x)
        #define MPACK_NHSWAP32(x) (x)
        #define MPACK_NHSWAP64(x) (x)
    #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

        #if !MPACK_NO_BUILTINS
            #if defined(__clang__)
                #ifdef __has_builtin
                    // Unlike the GCC builtins, the bswap builtins in Clang
                    // significantly improve ARM performance.
                    #if __has_builtin(__builtin_bswap16)
                        #define MPACK_NHSWAP16(x) __builtin_bswap16(x)
                    #endif
                    #if __has_builtin(__builtin_bswap32)
                        #define MPACK_NHSWAP32(x) __builtin_bswap32(x)
                    #endif
                    #if __has_builtin(__builtin_bswap64)
                        #define MPACK_NHSWAP64(x) __builtin_bswap64(x)
                    #endif
                #endif

            #elif defined(__GNUC__)

                // The GCC bswap builtins are apparently poorly optimized on older
                // versions of GCC, so we set a minimum version here just in case.
                //     http://hardwarebug.org/2010/01/14/beware-the-builtins/

                #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
                    #define MPACK_NHSWAP64(x) __builtin_bswap64(x)
                #endif

                // __builtin_bswap16() was not implemented on all platforms
                // until GCC 4.8.0:
                //     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
                //
                // The 16- and 32-bit versions in GCC significantly reduce performance
                // on ARM with little effect on code size so we don't use them.

            #endif
        #endif
    #endif

#elif defined(_MSC_VER) && defined(_WIN32) && !MPACK_NO_BUILTINS

    // On Windows, we assume x86 and x86_64 are always little-endian.
    // We make no assumptions about ARM even though all current
    // Windows Phone devices are little-endian in case Microsoft's
    // compiler is ever used with a big-endian ARM device.

    #if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
        #define MPACK_NHSWAP16(x) _byteswap_ushort(x)
        #define MPACK_NHSWAP32(x) _byteswap_ulong(x)
        #define MPACK_NHSWAP64(x) _byteswap_uint64(x)
    #endif

#endif

#if defined(__FLOAT_WORD_ORDER__) && defined(__BYTE_ORDER__)

    // We check where possible that the float byte order matches the
    // integer byte order. This is extremely unlikely to fail, but
    // we check anyway just in case.
    //
    // (The static assert is placed in float/double encoders instead
    // of here because our static assert fallback doesn't work at
    // file scope)

    #define MPACK_CHECK_FLOAT_ORDER() \
        MPACK_STATIC_ASSERT(__FLOAT_WORD_ORDER__ == __BYTE_ORDER__, \
            "float byte order does not match int byte order! float/double " \
            "encoding is not properly implemented on this platform.")

#endif

#ifndef MPACK_CHECK_FLOAT_ORDER
    #define MPACK_CHECK_FLOAT_ORDER() /* nothing */
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

    /**
     * @addtogroup config
     * @{
     */
    /**
     * @name Debug Functions
     */
    /**
     * Implement this and define @ref MPACK_CUSTOM_ASSERT to use a custom
     * assertion function.
     *
     * This function should not return. If it does, MPack will @c abort().
     *
     * If you use C++, make sure you include @c mpack.h where you define
     * this to get the correct linkage (or define it <code>extern "C"</code>.)
     *
     * Asserts are only used when @ref MPACK_DEBUG is enabled, and can be
     * triggered by bugs in MPack or bugs due to incorrect usage of MPack.
     */
    void mpack_assert_fail(const char* message);
    /**
     * @}
     */
    /**
     * @}
     */

    MPACK_NORETURN(void mpack_assert_fail_wrapper(const char* message));
    #if MPACK_STDIO
        MPACK_NORETURN(void mpack_assert_fail_format(const char* format, ...));
        #define mpack_assert_fail_at(line, file, expr, ...) \
                mpack_assert_fail_format("mpack assertion failed at " file ":" #line "\n" expr "\n" __VA_ARGS__)
    #else
        #define mpack_assert_fail_at(line, file, ...) \
                mpack_assert_fail_wrapper("mpack assertion failed at " file ":" #line )
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
    #define mpack_memcmp memcmp
    #define mpack_memcpy memcpy
    #define mpack_memmove memmove
    #define mpack_memset memset
    #ifndef mpack_strlen
        #define mpack_strlen strlen
    #endif

    #if defined(MPACK_UNIT_TESTS) && MPACK_INTERNAL && defined(__GNUC__)
        // make sure we don't use the stdlib directly during development
        #undef memcmp
        #undef memcpy
        #undef memmove
        #undef memset
        #undef strlen
        #undef malloc
        #undef free
        #pragma GCC poison memcmp
        #pragma GCC poison memcpy
        #pragma GCC poison memmove
        #pragma GCC poison memset
        #pragma GCC poison strlen
        #pragma GCC poison malloc
        #pragma GCC poison free
    #endif

#elif defined(__GNUC__) && !MPACK_NO_BUILTINS
    // there's not always a builtin memmove for GCC,
    // and we don't have a way to test for it
    #define mpack_memcmp __builtin_memcmp
    #define mpack_memcpy __builtin_memcpy
    #define mpack_memset __builtin_memset
    #define mpack_strlen __builtin_strlen

#elif defined(__clang__) && defined(__has_builtin) && !MPACK_NO_BUILTINS
    #if __has_builtin(__builtin_memcmp)
    #define mpack_memcmp __builtin_memcmp
    #endif
    #if __has_builtin(__builtin_memcpy)
    #define mpack_memcpy __builtin_memcpy
    #endif
    #if __has_builtin(__builtin_memmove)
    #define mpack_memmove __builtin_memmove
    #endif
    #if __has_builtin(__builtin_memset)
    #define mpack_memset __builtin_memset
    #endif
    #if __has_builtin(__builtin_strlen)
    #define mpack_strlen __builtin_strlen
    #endif
#endif

#ifndef mpack_memcmp
int mpack_memcmp(const void* s1, const void* s2, size_t n);
#endif
#ifndef mpack_memcpy
void* mpack_memcpy(void* MPACK_RESTRICT s1, const void* MPACK_RESTRICT s2, size_t n);
#endif
#ifndef mpack_memmove
void* mpack_memmove(void* s1, const void* s2, size_t n);
#endif
#ifndef mpack_memset
void* mpack_memset(void* s, int c, size_t n);
#endif
#ifndef mpack_strlen
size_t mpack_strlen(const char* s);
#endif

#if MPACK_STDIO
    #if defined(WIN32)
        #define mpack_snprintf _snprintf
    #else
        #define mpack_snprintf snprintf
    #endif
#endif



/* Debug logging */
#if 0
    #define mpack_log(...) (printf(__VA_ARGS__), fflush(stdout))
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
        MPACK_INLINE void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size) {
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

#endif

