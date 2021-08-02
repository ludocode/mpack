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

static void test_reader_should_inplace(void) {
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

static void test_reader_miscellaneous(void) {

    // 0xc1 is reserved; it should always raise mpack_error_invalid
    TEST_SIMPLE_READ_ERROR("\xc1", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_invalid);

    #if !MPACK_EXTENSIONS
    // ext types are unsupported without MPACK_EXTENSIONS
    TEST_SIMPLE_READ_ERROR("\xc7", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xc8", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xc9", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xd4", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xd5", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xd6", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xd7", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    TEST_SIMPLE_READ_ERROR("\xd8", mpack_tag_equal(mpack_read_tag(&reader), mpack_tag_nil()), mpack_error_unsupported);
    #endif

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
static void test_print_buffer(void) {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";

    char buffer[1024];
    mpack_print_data_to_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    static const char* expected =
        "{\n"
        "    \"compact\": true,\n"
        "    \"schema\": 0\n"
        "}";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}

static void test_print_buffer_bounds(void) {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";

    char buffer[10];
    mpack_print_data_to_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    // string should be truncated with a null-terminator
    static const char* expected = "{\n    \"co";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}

static void test_print_buffer_hexdump(void) {
    static const char test[] = "\xc4\x03""abc";

    char buffer[64];
    mpack_print_data_to_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    // string should be truncated with a null-terminator
    static const char* expected = "<binary data of length 3: 616263>";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}

static void test_print_buffer_no_hexdump(void) {
    static const char test[] = "\xc4\x00";

    char buffer[64];
    mpack_print_data_to_buffer(test, sizeof(test) - 1, buffer, sizeof(buffer));

    // string should be truncated with a null-terminator
    static const char* expected = "<binary data of length 0>";
    TEST_TRUE(buffer[strlen(expected)] == 0);
    TEST_TRUE(0 == strcmp(buffer, expected));
}
#endif

static bool count_messages(const void* buffer, size_t byte_count, size_t* message_count) {
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, byte_count);

    *message_count = 0;
    while (mpack_reader_remaining(&reader, NULL) > 0) {
        ++*message_count;
        mpack_discard(&reader);
    }

    return mpack_ok == mpack_reader_destroy(&reader);
}

static void test_count_messages(void) {
    static const char test[] = "\x80\x81\xA3""key\xA5""value\x92\xc2\xc3\x90";
    size_t message_count;
    TEST_TRUE(count_messages(test, sizeof(test)-1, &message_count));
    TEST_TRUE(message_count == 4);

    static const char test2[] = "\x92\xc0"; // truncated buffer
    TEST_TRUE(!count_messages(test2, sizeof(test2)-1, &message_count));
}

void test_reader() {
    #if MPACK_DEBUG && MPACK_STDIO
    test_print_buffer();
    test_print_buffer_bounds();
    test_print_buffer_hexdump();
    test_print_buffer_no_hexdump();
    #endif
    test_reader_should_inplace();
    test_reader_miscellaneous();
    test_count_messages();
}

#endif

