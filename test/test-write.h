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

#ifndef MPACK_TEST_WRITE_H
#define MPACK_TEST_WRITE_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_WRITER

extern mpack_error_t test_write_error;
void test_write_error_handler(mpack_writer_t* writer, mpack_error_t error);


// these setup and destroy test writers and check them for errors.
// they are generally macros so that the asserts are on the line of the test.


// tears down a writer, ensuring it didn't fail
#define TEST_WRITER_DESTROY_NOERROR(writer) \
    TEST_TRUE(mpack_writer_destroy(writer) == mpack_ok, \
            "writer is in error state %i", (int)mpack_writer_error(writer)); \

// tears down a writer, ensuring the given error occurred
#define TEST_WRITER_DESTROY_ERROR(writer, error) do { \
    mpack_error_t expected = (error); \
    mpack_error_t actual = mpack_writer_destroy(writer); \
    TEST_TRUE(actual == expected, "writer is in error state %i instead of %i", \
            (int)actual, (int)expected); \
} while (0)

// performs an operation on a writer, ensuring no error occurs
#define TEST_WRITE_NOERROR(writer, write_expr) do { \
    (write_expr); \
    TEST_TRUE(mpack_writer_error(writer) == mpack_ok, \
            "writer is in error state %i", (int)mpack_writer_error(writer)); \
} while (0)

#define TEST_DESTROY_MATCH_SIZE(expect, size) do { \
    static const char data[] = expect; \
    TEST_WRITER_DESTROY_NOERROR(&writer); \
    TEST_TRUE(sizeof(data)-1 == size, \
            "written data length %i does not match length %i of expected", \
            (int)size, (int)(sizeof(data)-1)); \
    TEST_TRUE(memcmp(data, buf, size) == 0, \
            "written data does not match expected"); \
} while (0)

#define TEST_DESTROY_MATCH(expect) do { \
    TEST_DESTROY_MATCH_SIZE(expect, size); \
    if (buf) MPACK_FREE(buf); \
} while (0)

// runs a simple writer test, ensuring it matches the given data
#define TEST_SIMPLE_WRITE(expect, write_op) do { \
    mpack_writer_t writer; \
    mpack_writer_init(&writer, buf, sizeof(buf)); \
    mpack_writer_set_error_handler(&writer, test_write_error_handler); \
    write_op; \
    TEST_DESTROY_MATCH_SIZE(expect, mpack_writer_buffer_used(&writer)); \
    TEST_TRUE(test_write_error == mpack_ok); \
    test_write_error = mpack_ok; \
} while (0)

// runs a simple writer test, ensuring it does not cause an error and ignoring the result
#define TEST_SIMPLE_WRITE_NOERROR(write_op) do { \
    mpack_writer_t writer; \
    mpack_writer_init(&writer, buf, sizeof(buf)); \
    mpack_writer_set_error_handler(&writer, test_write_error_handler); \
    (write_op); \
    TEST_WRITER_DESTROY_NOERROR(&writer); \
    TEST_TRUE(test_write_error == mpack_ok); \
    test_write_error = mpack_ok; \
} while (0)

// runs a simple writer test, ensuring it does causes the given error
#define TEST_SIMPLE_WRITE_ERROR(write_op, error) do { \
    mpack_error_t expected2 = (error); \
    mpack_writer_t writer; \
    mpack_writer_init(&writer, buf, sizeof(buf)); \
    mpack_writer_set_error_handler(&writer, test_write_error_handler); \
    (write_op); \
    TEST_WRITER_DESTROY_ERROR(&writer, expected2); \
    TEST_TRUE(test_write_error == expected2); \
    test_write_error = mpack_ok; \
} while (0)

void test_writes(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

