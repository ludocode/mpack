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

#include "test-expect.h"

#if MPACK_READER

mpack_error_t test_read_error = mpack_ok;

void test_read_error_handler(mpack_reader_t* reader, mpack_error_t error) {
    TEST_TRUE(test_read_error == mpack_ok, "error handler was called multiple times");
    TEST_TRUE(error != mpack_ok, "error handler was called with mpack_ok");
    TEST_TRUE(mpack_reader_error(reader) == error, "reader error does not match given error");
    test_read_error = error;
}

void test_reader() {
    // almost all reader functions are tested by the expect tests.
    // minor miscellaneous read tests are added here.

    // 0xc1 is reserved; it should always raise mpack_error_invalid
    TEST_SIMPLE_READ_ERROR("\xc1", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);

    // truncated discard errors
    TEST_SIMPLE_READ_ERROR("\x91", (mpack_discard(&reader), true), mpack_error_invalid); // array
    TEST_SIMPLE_READ_ERROR("\x81", (mpack_discard(&reader), true), mpack_error_invalid); // map
}

#endif

