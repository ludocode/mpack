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
 * Now that the config is included, we undef any of the configs
 * that aren't true. This allows defining configs to 0 without
 * us having to write "#if defined(X) && X" everywhere.
 */
#if defined(MPACK_READER) && !MPACK_READER
#undef MPACK_READER
#endif
#if defined(MPACK_EXPECT) && !MPACK_EXPECT
#undef MPACK_EXPECT
#endif
#if defined(MPACK_NODE) && !MPACK_NODE
#undef MPACK_NODE
#endif
#if defined(MPACK_WRITER) && !MPACK_WRITER
#undef MPACK_WRITER
#endif
#if defined(MPACK_STDLIB) && !MPACK_STDLIB
#undef MPACK_STDLIB
#endif
#if defined(MPACK_STDIO) && !MPACK_STDIO
#undef MPACK_STDIO
#endif
#if defined(MPACK_DEBUG) && !MPACK_DEBUG
#undef MPACK_DEBUG
#endif
#if defined(MPACK_CUSTOM_ASSERT) && !MPACK_CUSTOM_ASSERT
#undef MPACK_CUSTOM_ASSERT
#endif
#if defined(MPACK_READ_TRACKING) && !MPACK_READ_TRACKING
#undef MPACK_READ_TRACKING
#endif
#if defined(MPACK_WRITE_TRACKING) && !MPACK_WRITE_TRACKING
#undef MPACK_WRITE_TRACKING
#endif
#if defined(MPACK_NO_TRACKING) && !MPACK_NO_TRACKING
#undef MPACK_NO_TRACKING
#endif
#ifndef MPACK_OPTIMIZE_FOR_SIZE
#define MPACK_OPTIMIZE_FOR_SIZE 0
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

#ifdef MPACK_STDLIB
#include <string.h>
#include <stdlib.h>
#endif
#ifdef MPACK_STDIO
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif



/* Miscellaneous helper macros */

#define MPACK_UNUSED(var) ((void)(var))

#ifdef MPACK_AMALGAMATED
#define MPACK_INTERNAL_STATIC static
#else
#define MPACK_INTERNAL_STATIC
#endif

#define MPACK_STRINGIFY_IMPL(arg) #arg
#define MPACK_STRINGIFY(arg) MPACK_STRINGIFY_IMPL(arg)



/*
 * Definition of inline macros.
 *
 * MPack supports several different modes for inline functions:
 *   - functions force-inlined (MPACK_ALWAYS_INLINE)
 *   - functions declared inline regardless of optimization options (MPACK_INLINE)
 *   - functions declared inline only in builds optimized for speed (MPACK_INLINE_SPEED)
 *
 * MPack is currently transitioning away from using "static inline". Only one
 * non-inline definition of each function should exist in the final build, and
 * comparing addresses of functions should compare equal regardless of whether
 * they are declared inline.
 *
 * The above requirements mean that the declaration and definition of most
 * inline functions must be separated so that the definitions will only
 * appear when necessary. In addition, three different linkage models need
 * to be supported:
 *
 *  - The C99 model, where "inline" does not emit a definition and "extern inline" does
 *  - The GNU model, where "inline" emits a definition and "extern inline" does not
 *  - The C++ model, where "inline" emits a definition with weak linkage
 *
 * The macros below wrap up everything above. All inline functions have a single
 * non-inline definition emitted in the compilation of mpack-platform.c.
 */

#if defined(__cplusplus)
    // C++ rules
    // The linker will need weak symbol support to link C++ object files,
    // so we don't need to worry about emitting a single definition.
    #define MPACK_INLINE inline
#elif defined(__GNUC__) && (defined(__GNUC_GNU_INLINE__) || \
        !defined(__GNUC_STDC_INLINE__) && !defined(__GNUC_GNU_INLINE__))
    // GNU rules
    #ifdef MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE inline
    #else
        #define MPACK_INLINE extern inline
    #endif
#else
    // C99 rules
    #ifdef MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE extern inline
    #else
        #define MPACK_INLINE inline
    #endif
#endif

#if MPACK_OPTIMIZE_FOR_SIZE
    #define MPACK_INLINE_SPEED /* nothing */
    #ifdef MPACK_EMIT_INLINE_DEFS
        #define MPACK_DEFINE_INLINE_SPEED 1
    #else
        #define MPACK_DEFINE_INLINE_SPEED 0
    #endif
#else
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
    #define MPACK_ALWAYS_INLINE __attribute__((always_inline)) static inline
#elif defined(_MSC_VER)
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

#ifdef MPACK_DEBUG
    MPACK_NORETURN(void mpack_assert_fail(const char* message));
    #ifdef MPACK_STDIO
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
    #ifdef MPACK_STDIO
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
#ifdef MPACK_STDLIB
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
#if defined(MPACK_READ_TRACKING) && !defined(MPACK_READER)
    #error "MPACK_READ_TRACKING requires MPACK_READER."
#endif
#if defined(MPACK_WRITE_TRACKING) && !defined(MPACK_WRITER)
    #error "MPACK_WRITE_TRACKING requires MPACK_WRITER."
#endif
#ifndef MPACK_MALLOC
    #ifdef MPACK_STDIO
    #error "MPACK_STDIO requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #ifdef MPACK_READ_TRACKING
    #error "MPACK_READ_TRACKING requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #ifdef MPACK_WRITE_TRACKING
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

/** @endcond */

#endif

