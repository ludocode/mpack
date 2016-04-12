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


// We define MPACK_EMIT_INLINE_DEFS and include mpack.h to emit
// standalone definitions of all (non-static) inline functions in MPack.

#define MPACK_INTERNAL 1
#define MPACK_EMIT_INLINE_DEFS 1

#include "mpack-platform.h"
#include "mpack.h"


#if MPACK_DEBUG && MPACK_STDIO
#include <stdarg.h>
#endif



#if MPACK_DEBUG

#if MPACK_STDIO
void mpack_assert_fail_format(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = 0;
    mpack_assert_fail_wrapper(buffer);
}

void mpack_break_hit_format(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = 0;
    mpack_break_hit(buffer);
}
#endif

#if !MPACK_CUSTOM_ASSERT
void mpack_assert_fail(const char* message) {
    MPACK_UNUSED(message);

    #if MPACK_STDIO
    fprintf(stderr, "%s\n", message);
    #endif
}
#endif

// We split the assert failure from the wrapper so that a
// custom assert function can return.
void mpack_assert_fail_wrapper(const char* message) {
    mpack_assert_fail(message);

    #if !MPACK_NO_BUILTINS
    #if defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
    #elif defined(WIN32)
    __debugbreak();
    #endif
    #endif

    #if (defined(__GNUC__) || defined(__clang__)) && !MPACK_NO_BUILTINS
    __builtin_abort();
    #elif MPACK_STDLIB
    abort();
    #endif

    MPACK_UNREACHABLE;
}

#if !MPACK_CUSTOM_BREAK

// If we have a custom assert handler, break wraps it by default.
// This allows users of MPack to only implement mpack_assert_fail() without
// having to worry about the difference between assert and break.
//
// MPACK_CUSTOM_BREAK is available to define a separate break handler
// (which is needed by the unit test suite), but this is not offered in
// mpack-config.h for simplicity.

#if MPACK_CUSTOM_ASSERT
void mpack_break_hit(const char* message) {
    mpack_assert_fail_wrapper(message);
}
#else
void mpack_break_hit(const char* message) {
    MPACK_UNUSED(message);

    #if MPACK_STDIO
    fprintf(stderr, "%s\n", message);
    #endif

    #if defined(__GNUC__) || defined(__clang__) && !MPACK_NO_BUILTINS
    __builtin_trap();
    #elif defined(WIN32) && !MPACK_NO_BUILTINS
    __debugbreak();
    #elif MPACK_STDLIB
    abort();
    #endif
}
#endif

#endif

#endif



// The below are adapted from the C wikibook:
//     https://en.wikibooks.org/wiki/C_Programming/Strings

#ifndef mpack_memcmp
int mpack_memcmp(const void* s1, const void* s2, size_t n) {
     const unsigned char *us1 = (const unsigned char *) s1;
     const unsigned char *us2 = (const unsigned char *) s2;
     while (n-- != 0) {
         if (*us1 != *us2)
             return (*us1 < *us2) ? -1 : +1;
         us1++;
         us2++;
     }
     return 0;
}
#endif

#ifndef mpack_memcpy
void* mpack_memcpy(void* MPACK_RESTRICT s1, const void* MPACK_RESTRICT s2, size_t n) {
    char* MPACK_RESTRICT dst = (char *)s1;
    const char* MPACK_RESTRICT src = (const char *)s2;
    while (n-- != 0)
        *dst++ = *src++;
    return s1;
}
#endif

#ifndef mpack_memmove
void* mpack_memmove(void* s1, const void* s2, size_t n) {
    char *p1 = (char *)s1;
    const char *p2 = (const char *)s2;
    if (p2 < p1 && p1 < p2 + n) {
        p2 += n;
        p1 += n;
        while (n-- != 0)
            *--p1 = *--p2;
    } else
        while (n-- != 0)
            *p1++ = *p2++;
    return s1;
}
#endif

#ifndef mpack_memset
void* mpack_memset(void* s, int c, size_t n) {
    unsigned char *us = (unsigned char *)s;
    unsigned char uc = (unsigned char)c;
    while (n-- != 0)
        *us++ = uc;
    return s;
}
#endif

#ifndef mpack_strlen
size_t mpack_strlen(const char* s) {
    const char* p = s;
    while (*p != '\0')
        p++;
    return (size_t)(p - s);
}
#endif



#if defined(MPACK_MALLOC) && !defined(MPACK_REALLOC)
void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size) {
    if (new_size == 0) {
        if (old_ptr)
            MPACK_FREE(old_ptr);
        return NULL;
    }

    void* new_ptr = MPACK_MALLOC(new_size);
    if (new_ptr == NULL)
        return NULL;

    mpack_memcpy(new_ptr, old_ptr, used_size);
    MPACK_FREE(old_ptr);
    return new_ptr;
}
#endif
