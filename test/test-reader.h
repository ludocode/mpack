/*
 * Copyright (c) 2015-2018 Nicholas Fraser
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

#ifndef MPACK_TEST_READER_H
#define MPACK_TEST_READER_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_READER

extern mpack_error_t test_read_error;
void test_read_error_handler(mpack_reader_t* reader, mpack_error_t error);


// these setup and destroy test readers and check them for errors.
// they are generally macros so that the checks are on the line of the test.


// destroy wrappers expecting error or no error

// tears down a reader, ensuring it has no errors and no extra data
#define TEST_READER_DESTROY_NOERROR(reader) do { \
    size_t remaining = mpack_reader_remaining(reader, NULL); \
    mpack_error_t error = mpack_reader_destroy(reader); \
    TEST_TRUE(error == mpack_ok, "reader is in error state %i (%s)", \
            (int)error, mpack_error_to_string(error)); \
    TEST_TRUE(remaining == 0, \
            "reader has %i extra bytes", (int)remaining); \
} while (0)

// tears down a reader, ensuring it is in the given error state
#define TEST_READER_DESTROY_ERROR(reader, error) do { \
    mpack_error_t expected = (error); \
    mpack_error_t actual = mpack_reader_destroy(reader); \
    TEST_TRUE(actual == expected, "reader is in error state %i (%s) instead of %i (%s)", \
            (int)actual, mpack_error_to_string(actual), \
            (int)expected, mpack_error_to_string(expected)); \
} while (0)



// reader helpers

// performs an operation on a reader, ensuring no error occurs
#define TEST_READ_NOERROR(reader, read_expr) do { \
    TEST_TRUE((read_expr), "read did not pass: " #read_expr); \
    TEST_TRUE(mpack_reader_error(reader) == mpack_ok, \
            "reader flagged error %i", (int)mpack_reader_error(reader)); \
} while (0)



// simple read tests

// initializes a reader from a literal string
#define TEST_READER_INIT_STR(reader, data) \
    mpack_reader_init_data(reader, data, sizeof(data) - 1)

// runs a simple reader test, ensuring the expression is true and no errors occur
#define TEST_SIMPLE_READ(data, read_expr) do { \
    TEST_READER_INIT_STR(&reader, data); \
    mpack_reader_set_error_handler(&reader, test_read_error_handler); \
    TEST_TRUE((read_expr), "simple read test did not pass: " #read_expr); \
    TEST_READER_DESTROY_NOERROR(&reader); \
    TEST_TRUE(test_read_error == mpack_ok); \
    test_read_error = mpack_ok; \
} while (0)

// runs a simple reader test, ensuring the expression is true and no errors occur, cancelling to ignore tracking info
#define TEST_SIMPLE_READ_CANCEL(data, read_expr) do { \
    TEST_READER_INIT_STR(&reader, data); \
    TEST_TRUE((read_expr), "simple read test did not pass: " #read_expr); \
    mpack_reader_flag_error(&reader, mpack_error_data); \
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_data); \
} while (0)

// runs a simple reader test, ensuring the expression is true and the given error is raised
#define TEST_SIMPLE_READ_ERROR(data, read_expr, error) do { \
    TEST_READER_INIT_STR(&reader, data); \
    mpack_reader_set_error_handler(&reader, test_read_error_handler); \
    TEST_TRUE((read_expr), "simple read error test did not pass: " #read_expr); \
    TEST_READER_DESTROY_ERROR(&reader, (error)); \
    TEST_TRUE(test_read_error == (error)); \
    test_read_error = mpack_ok; \
} while (0)



// simple read bug tests

#if MPACK_DEBUG

// runs a simple reader test, ensuring it causes an assert.
// we flag mpack_error_data to cancel out of any tracking.
// (note about volatile, see TEST_ASSERT())
#define TEST_SIMPLE_READ_ASSERT(data, read_expr) do { \
    volatile mpack_reader_t v_reader; \
    TEST_MPACK_SILENCE_SHADOW_BEGIN \
    mpack_reader_t* reader = (mpack_reader_t*)(uintptr_t)&v_reader; \
    TEST_MPACK_SILENCE_SHADOW_END \
    mpack_reader_init_data(reader, data, sizeof(data) - 1); \
    TEST_ASSERT(read_expr); \
    mpack_reader_flag_error(reader, mpack_error_data); \
    mpack_reader_destroy(reader); \
} while (0)

#else

// we cannot test asserts in release mode because they are
// compiled away; code would continue to run and cause
// undefined behavior.
#define TEST_SIMPLE_READ_ASSERT(data, read_expr) ((void)0)

#endif

// runs a simple reader test, ensuring it causes a break in
// debug mode and flags mpack_error_bug in both debug and release.
#define TEST_SIMPLE_READ_BREAK(data, read_expr) do { \
    mpack_reader_init_data(&reader, data, sizeof(data) - 1); \
    TEST_BREAK(read_expr); \
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug); \
} while (0)



void test_reader(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

