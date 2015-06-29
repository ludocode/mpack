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

#ifndef MPACK_TEST_WRITE_H
#define MPACK_TEST_WRITE_H 1

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_WRITER

// these setup and destroy test writers and check them for errors.
// they are generally macros so that the asserts are on the line of the test.

// tears down a writer, ensuring it didn't fail
#define test_writer_destroy_noerror(writer) \
    test_assert(mpack_writer_destroy(writer) == mpack_ok, \
            "writer is in error state %i", (int)mpack_writer_error(writer)); \

// performs an operation on a writer, ensuring no error occurs
#define test_write_noerror(writer, write_expr) do { \
    (write_expr); \
    test_assert(mpack_writer_error(writer) == mpack_ok, \
            "writer is in error state %i", (int)mpack_writer_error(writer)); \
} while (0)

#define test_destroy_match_size(expect, size) do { \
    static const char data[] = expect; \
    test_writer_destroy_noerror(&writer); \
    test_assert(sizeof(data)-1 == size, \
            "written data length %i does not match length %i of expected", \
            (int)size, (int)(sizeof(data)-1)); \
    test_assert(memcmp((data), buf, size) == 0, \
            "written data does not match expected"); \
} while (0)

#define test_destroy_match(expect) do { \
    test_destroy_match_size(expect, size); \
    if (buf) MPACK_FREE(buf); \
} while (0)

// runs a simple writer test, ensuring it matches the given data
#define test_simple_write(expect, write_op) do { \
    mpack_writer_t writer; \
    mpack_writer_init(&writer, buf, sizeof(buf)); \
    (write_op); \
    test_destroy_match_size(expect, mpack_writer_buffer_used(&writer)); \
} while (0)

void test_writes(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

