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

#ifndef MPACK_TEST_READER_H
#define MPACK_TEST_READER_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MPACK_READER

// these setup and destroy test readers and check them for errors.
// they are generally macros so that the asserts are on the line of the test.



// destroy wrappers expecting error or no error

// tears down a reader, ensuring it has no errors and no extra data
#define test_reader_destroy_noerror(reader) do { \
    test_assert(mpack_reader_error(reader) == mpack_ok, "reader is in error state %i (%s)", \
            (int)mpack_reader_error(reader), mpack_error_to_string(mpack_reader_error(reader))); \
    test_assert(mpack_reader_remaining(reader, NULL) == 0, \
            "reader has %i extra bytes", (int)mpack_reader_remaining(reader, NULL)); \
    mpack_reader_destroy(reader); \
} while (0)

// tears down a reader with tracking cancelling, ensuring it has no errors
#define test_reader_destroy_cancel_noerror(reader) do { \
    test_assert(mpack_reader_error(reader) == mpack_ok, "reader is in error state %i (%s)", \
            (int)mpack_reader_error(reader), mpack_error_to_string(mpack_reader_error(reader))); \
    mpack_reader_destroy_cancel(reader); \
} while (0)

// tears down a reader, ensuring it is in the given error state
#define test_reader_destroy_error(reader, error) do { \
    mpack_error_t e = (error); \
    test_assert(mpack_reader_error(reader) == e, "reader is in error state %i (%s) instead of %i (%s)", \
            (int)mpack_reader_error(reader), mpack_error_to_string(mpack_reader_error(reader)), \
            (int)e, mpack_error_to_string(e)); \
    mpack_reader_destroy(reader); \
} while (0)



// reader helpers

// performs an operation on a reader, ensuring no error occurs
#define test_read_noerror(reader, read_expr) do { \
    test_assert((read_expr), "read did not pass: " #read_expr); \
    test_assert(mpack_reader_error(reader) == mpack_ok, \
            "reader flagged error %i", (int)mpack_reader_error(reader)); \
} while (0)



// simple read tests

// initializes a reader from a literal string
#define test_reader_init_str(reader, data) \
    mpack_reader_init_data(reader, data, sizeof(data) - 1)

// runs a simple reader test, ensuring the expression is true and no errors occur
#define test_simple_read(data, read_expr) do { \
    mpack_reader_t reader; \
    test_reader_init_str(&reader, data); \
    test_assert((read_expr), "simple read test did not pass: " #read_expr); \
    test_reader_destroy_noerror(&reader); \
} while (0)

// runs a simple reader test, ensuring the expression is true and no errors occur, cancelling to ignore tracking info
#define test_simple_read_cancel(data, read_expr) do { \
    mpack_reader_t reader; \
    test_reader_init_str(&reader, data); \
    test_assert((read_expr), "simple read test did not pass: " #read_expr); \
    test_reader_destroy_cancel_noerror(&reader); \
} while (0)

// runs a simple reader test, ensuring the expression is true and the given error is raised
#define test_simple_read_error(data, read_expr, error) do { \
    mpack_reader_t reader; \
    test_reader_init_str(&reader, data); \
    test_assert((read_expr), "simple read error test did not pass: " #read_expr); \
    test_reader_destroy_error(&reader, (error)); \
} while (0)



// simple read assertion test

// runs a simple reader test, ensuring it causes an assert.
// (note about volatile, see test_expecting_assert())
// (we cancel in case the mpack_error_bug is compiled out in release mode)
#define test_simple_read_assert(data, read_expr) do { \
    volatile mpack_reader_t v_reader; \
    mpack_reader_t* reader = (mpack_reader_t*)&v_reader; \
    mpack_reader_init_data(reader, data, sizeof(data) - 1); \
    test_expecting_assert(read_expr); \
    mpack_reader_destroy_cancel(reader); \
} while (0)

// runs a simple reader test, ensuring it causes a break.
// (we cancel in case the mpack_error_bug is compiled out in release mode)
#define test_simple_read_break(data, read_expr) do { \
    mpack_reader_t reader; \
    mpack_reader_init_data(&reader, data, sizeof(data) - 1); \
    test_expecting_break(read_expr); \
    mpack_reader_destroy_cancel(&reader); \
} while (0)



void test_reader(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

