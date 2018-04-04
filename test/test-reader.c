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

#include "test-expect.h"

#if MPACK_READER

mpack_error_t test_read_error = mpack_ok;

void test_read_error_handler(mpack_reader_t* reader, mpack_error_t error) {
    TEST_TRUE(test_read_error == mpack_ok, "error handler was called multiple times");
    TEST_TRUE(error != mpack_ok, "error handler was called with mpack_ok");
    TEST_TRUE(mpack_reader_error(reader) == error, "reader error does not match given error");
    test_read_error = error;
}

// almost all reader functions are tested by the expect tests.
// minor miscellaneous read tests are added here.

static void test_reader_should_inplace() {
    char buf[4096];
    mpack_reader_t reader;
    mpack_reader_init(&reader, buf, sizeof(buf), 0);

    TEST_TRUE(true  == mpack_should_read_bytes_inplace(&reader, 0));
    TEST_TRUE(true  == mpack_should_read_bytes_inplace(&reader, 1));
    TEST_TRUE(true  == mpack_should_read_bytes_inplace(&reader, 20));
    TEST_TRUE(false == mpack_should_read_bytes_inplace(&reader, 500));
    TEST_TRUE(false == mpack_should_read_bytes_inplace(&reader, 10000));

    mpack_reader_destroy(&reader);
}

static void test_reader_miscellaneous() {

    // 0xc1 is reserved; it should always raise mpack_error_invalid
    TEST_SIMPLE_READ_ERROR("\xc1", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);

    // simple truncated tags (testing discard of additional
    // temporary data in mpack_parse_tag())
    TEST_SIMPLE_READ_ERROR("\xcc", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xcd", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xce", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xcf", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xd0", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xd1", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xd2", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("\xd3", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);

    // truncated discard errors
    TEST_SIMPLE_READ_ERROR("\x91", (mpack_discard(&reader), true), mpack_error_invalid); // array
    TEST_SIMPLE_READ_ERROR("\x81", (mpack_discard(&reader), true), mpack_error_invalid); // map
}

#if MPACK_DEBUG && MPACK_STDIO
static void test_print_buffer() {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";

    char buffer[1024];
    mpack_print_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    static const char* expected =
        "{\n"
        "    \"compact\": true,\n"
        "    \"schema\": 0\n"
        "}";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}

static void test_print_buffer_bounds() {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";

    char buffer[10];
    mpack_print_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    // string should be truncated with a null-terminator
    static const char* expected = "{\n    \"co";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}
#endif

void test_reader() {
    #if MPACK_DEBUG && MPACK_STDIO
    test_print_buffer();
    test_print_buffer_bounds();
    #endif
    test_reader_should_inplace();
    test_reader_miscellaneous();
}

#endif

