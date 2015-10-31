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

#ifndef MPACK_TEST_H
#define MPACK_TEST_H 1

#define _DEFAULT_SOURCE 1
#define _BSD_SOURCE 1

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <setjmp.h>

#ifdef WIN32
#include <float.h>
#define isnanf _isnan
#endif

#ifdef __APPLE__
#define isnanf isnan
#endif

#ifdef WIN32
#include <direct.h>
#define mkdir(path, mode) ((void)(mode), _mkdir(path))
#define rmdir _rmdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __cplusplus
#include <limits>
#define MPACK_INFINITY std::numeric_limits<float>::infinity()
#else
#define MPACK_INFINITY INFINITY
#endif

#include "mpack/mpack.h"

extern mpack_tag_t (*fn_mpack_tag_nil)(void);

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is basically the whole unit testing framework for mpack.
 * The reported number of "tests" is the total number of test asserts,
 * where each actual test case has several asserts.
 */

// enable this to exit at the first error
#define TEST_EARLY_EXIT 1

// runs the given expression, causing a unit test failure with the
// given printf format string if the expression is not true.
#define TEST_TRUE(expr, ...) \
    TEST_TRUE_impl((expr), __FILE__, __LINE__, " " __VA_ARGS__)

void TEST_TRUE_impl(bool result, const char* file, int line, const char* format, ...);

extern int tests;
extern int passes;

#if MPACK_CUSTOM_ASSERT
extern bool test_jmp_set;
extern jmp_buf test_jmp_buf;
extern bool test_break_set;
extern bool test_break_hit;

// calls setjmp to expect an assert from a unit test. an assertion
// will cause a longjmp to here with a value of 1.
#define TEST_TRUE_SETJMP() \
    (TEST_TRUE(!test_jmp_set, "an assert jmp is already set!"), \
        test_jmp_set = true, \
        setjmp(test_jmp_buf))

// clears the expectation of an assert. a subsequent assert will
// cause the unit test suite to abort with error.
#define TEST_TRUE_CLEARJMP() \
    (TEST_TRUE(test_jmp_set, "an assert jmp is not set!"), \
        test_jmp_set = false)

// runs the given expression, causing a unit test failure if an assertion
// is not triggered. (note that stack variables may need to be marked volatile
// since non-volatile stack variables that are written to after setjmp are
// undefined after longjmp.)
#define TEST_ASSERT(expr) do { \
    volatile bool jumped = false; \
    if (TEST_TRUE_SETJMP()) { \
        jumped = true; \
    } else { \
        (expr); \
    } \
    TEST_TRUE_CLEARJMP(); \
    TEST_TRUE(jumped, "expression should assert, but didn't: " #expr); \
} while (0)

#define TEST_BREAK(expr, ...) do { \
    test_break_set = true; \
    test_break_hit = false; \
    TEST_TRUE(expr , ## __VA_ARGS__, "expression is not true: " # expr); \
    TEST_TRUE(test_break_hit, "expression should break, but didn't: " # expr); \
    test_break_set = false; \
} while (0);

#else

// in release mode we just run the expr since asserts are compiled out.
#define TEST_ASSERT(expr) do { (expr); } while (0)
#define TEST_BREAK(expr, ...) do { TEST_TRUE(expr , ## __VA_ARGS__); } while (0)

#endif

#ifdef __cplusplus
}
#endif

#endif

