/*
 * Copyright (c) 2015-2019 Nicholas Fraser
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

#include "test-builder.h"
#include "test-write.h"

#if MPACK_BUILDER
static void test_builder_basic(void) {
    char buf[4096];
    mpack_writer_t writer;

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_build_array(&writer);
    mpack_complete_array(&writer);
    TEST_DESTROY_MATCH_IMPL("\x90");

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_build_array(&writer);
    mpack_write_u8(&writer, 2);
    mpack_complete_array(&writer);
    TEST_DESTROY_MATCH_IMPL("\x91\x02");

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_build_map(&writer);
    mpack_write_cstr(&writer, "hello");
    mpack_write_cstr(&writer, "world");
    mpack_complete_map(&writer);
    TEST_DESTROY_MATCH_IMPL("\x81\xa5hello\xa5world");
}

static void test_builder_repeat(void) {
    char buf[4096];
    mpack_writer_t writer;

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 4);
    mpack_build_array(&writer);
    mpack_complete_array(&writer);
    mpack_build_map(&writer);
    mpack_complete_map(&writer);
    mpack_build_array(&writer);
    mpack_write_u8(&writer, 2);
    mpack_complete_array(&writer);
    mpack_build_map(&writer);
    mpack_write_cstr(&writer, "hello");
    mpack_write_cstr(&writer, "world");
    mpack_complete_map(&writer);
    mpack_finish_array(&writer);

    TEST_DESTROY_MATCH_IMPL("\x94\x90\x80\x91\x02\x81\xa5hello\xa5world");
}

static void test_builder_nested(void) {
    char buf[4096];
    mpack_writer_t writer;

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_build_map(&writer);
    mpack_write_cstr(&writer, "nums");
    mpack_build_array(&writer);
    mpack_write_int(&writer, 1);
    mpack_write_int(&writer, 2);
    mpack_write_int(&writer, 3);
    mpack_complete_array(&writer);
    mpack_write_cstr(&writer, "nil");
    mpack_write_nil(&writer);
    mpack_complete_map(&writer);
    TEST_DESTROY_MATCH_IMPL("\x82\xa4nums\x93\x01\x02\x03\xa3nil\xc0");

    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_build_array(&writer);
    mpack_build_array(&writer);
    mpack_build_array(&writer);
    mpack_write_int(&writer, 1);
    mpack_write_int(&writer, 2);
    mpack_write_int(&writer, 3);
    mpack_complete_array(&writer);
    mpack_complete_array(&writer);
    mpack_complete_array(&writer);
    TEST_DESTROY_MATCH_IMPL("\x91\x91\x93\x01\x02\x03");
}

static void test_builder_deep(void) {
    char buf[16*1024];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buf, sizeof(buf));

    char expected[sizeof(buf)];
    size_t pos = 0;
    int depth = 2;//50;

    int i;
    for (i = 0; i < depth; ++i) {
        //mpack_build_map(&writer);
        mpack_start_map(&writer, 2);
        expected[pos++] = '\x82';
        mpack_write_cstr(&writer, "ab");
        expected[pos++] = '\xa2';
        expected[pos++] = 'a';
        expected[pos++] = 'b';
        mpack_build_array(&writer);
        expected[pos++] = '\x94';
        mpack_write_int(&writer, 2);
        expected[pos++] = '\x02';
        mpack_write_int(&writer, 3);
        expected[pos++] = '\x03';
        mpack_write_int(&writer, 4);
        expected[pos++] = '\x04';
    }

    mpack_write_bool(&writer, true);
    expected[pos++] = '\xc3';

    for (i = 0; i < depth; ++i) {
        mpack_complete_array(&writer);
        mpack_write_int(&writer, 1);
        expected[pos++] = '\x01';
        mpack_write_nil(&writer);
        expected[pos++] = '\xc0';
        //mpack_complete_map(&writer);
        mpack_finish_map(&writer);
    }

    TEST_TRUE(pos <= sizeof(expected));
    size_t used = mpack_writer_buffer_used(&writer);

    /*
    printf("actual %zi expected %zi\n", used, pos);
    for (size_t i = 0; i < used; ++i) {
        printf("%02hhx ", buf[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    printf("\n");
    for (size_t i = 0; i < pos; ++i) {
        printf("%02hhx ", expected[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    */

    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(used == pos);
    TEST_TRUE(0 == memcmp(buf, expected, used));
}

static void test_builder_large(void) {
    char buf[16*1024];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buf, sizeof(buf));

    char expected[sizeof(buf)];
    size_t pos = 0;
    int depth = 6;

    int i;
    for (i = 0; i < depth; ++i) {
        mpack_build_map(&writer);
        expected[pos++] = '\xde';
        expected[pos++] = '\x00';
        expected[pos++] = '\x32';
        size_t j;
        for (j = 0; j < 99; ++j) {
            mpack_write_int(&writer, -1);
            expected[pos++] = '\xff';
        }
    }

    mpack_write_int(&writer, -1);
    expected[pos++] = '\xff';

    for (i = 0; i < depth; ++i) {
        mpack_complete_map(&writer);
    }

    TEST_TRUE(pos <= sizeof(expected));
    size_t used = mpack_writer_buffer_used(&writer);
    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(used == pos);
    TEST_TRUE(0 == memcmp(buf, expected, used));
}

static void test_builder_content(void) {
    char buf[16*1024];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buf, sizeof(buf));

    char expected[sizeof(buf)];
    size_t pos = 0;

    mpack_build_map(&writer);
    //mpack_start_map(&writer, 3);
    expected[pos++] = '\x83';

    mpack_write_cstr(&writer, "rid");
    expected[pos++] = '\xa3';
    expected[pos++] = 'r';
    expected[pos++] = 'i';
    expected[pos++] = 'd';

    char rid[16] = {0};
    mpack_write_bin(&writer, rid, sizeof(rid));
    expected[pos++] = '\xc4';
    expected[pos++] = '\x10';
    size_t i;
    for (i = 0; i < 16; ++i)
        expected[pos++] = '\x00';

    mpack_write_cstr(&writer, "type");
    expected[pos++] = '\xa4';
    expected[pos++] = 't';
    expected[pos++] = 'y';
    expected[pos++] = 'p';
    expected[pos++] = 'e';

    mpack_write_cstr(&writer, "inode");
    expected[pos++] = '\xa5';
    expected[pos++] = 'i';
    expected[pos++] = 'n';
    expected[pos++] = 'o';
    expected[pos++] = 'd';
    expected[pos++] = 'e';

    mpack_write_cstr(&writer, "content");
    expected[pos++] = '\xa7';
    expected[pos++] = 'c';
    expected[pos++] = 'o';
    expected[pos++] = 'n';
    expected[pos++] = 't';
    expected[pos++] = 'e';
    expected[pos++] = 'n';
    expected[pos++] = 't';

    mpack_start_map(&writer, 3);
    expected[pos++] = '\x83';

    mpack_write_cstr(&writer, "path");
    expected[pos++] = '\xa4';
    expected[pos++] = 'p';
    expected[pos++] = 'a';
    expected[pos++] = 't';
    expected[pos++] = 'h';

    mpack_write_cstr(&writer, "IMG_2445.JPG");
    expected[pos++] = '\xac';
    expected[pos++] = 'I';
    expected[pos++] = 'M';
    expected[pos++] = 'G';
    expected[pos++] = '_';
    expected[pos++] = '2';
    expected[pos++] = '4';
    expected[pos++] = '4';
    expected[pos++] = '5';
    expected[pos++] = '.';
    expected[pos++] = 'J';
    expected[pos++] = 'P';
    expected[pos++] = 'G';

    mpack_write_cstr(&writer, "parent");
    expected[pos++] = '\xa6';
    expected[pos++] = 'p';
    expected[pos++] = 'a';
    expected[pos++] = 'r';
    expected[pos++] = 'e';
    expected[pos++] = 'n';
    expected[pos++] = 't';

    mpack_write_bin(&writer, rid, sizeof(rid));
    expected[pos++] = '\xc4';
    expected[pos++] = '\x10';
    for (i = 0; i < 16; ++i)
        expected[pos++] = '\x00';

    mpack_write_cstr(&writer, "pass");
    expected[pos++] = '\xa4';
    expected[pos++] = 'p';
    expected[pos++] = 'a';
    expected[pos++] = 's';
    expected[pos++] = 's';

    mpack_write_int(&writer, 0);
    expected[pos++] = '\x00';

    mpack_finish_map(&writer);

    //mpack_finish_map(&writer);
    mpack_complete_map(&writer);

    TEST_TRUE(pos <= sizeof(expected));
    size_t used = mpack_writer_buffer_used(&writer);

    /*
    printf("actual %zi expected %zi\n", used, pos);
    for (i = 0; i < used; ++i) {
        printf("%02hhx ", buf[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    printf("\n");
    for (i = 0; i < pos; ++i) {
        printf("%02hhx ", expected[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    */

    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(used == pos);
    TEST_TRUE(0 == memcmp(buf, expected, used));
}

static void test_builder_add_expected_str(char* expected, size_t* pos, const char* str, uint32_t length) {
    if (length <= 31) {
        expected[(*pos)++] = (char)((uint32_t)'\xa0' + length);
    } else if (length <= MPACK_UINT8_MAX) {
        expected[(*pos)++] = '\xd9';
        expected[(*pos)++] = (char)(uint8_t)length;
    } else {
        expected[(*pos)++] = '\xda';
        expected[(*pos)++] = (char)(uint8_t)(length >> 8);
        expected[(*pos)++] = (char)(uint8_t)(length);
    }
    memcpy(expected + *pos, str, length);
    *pos += length;
}

static void test_builder_strings_length(uint32_t length) {
    char buf[16*1024];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buf, sizeof(buf));

    char expected[sizeof(buf)];
    size_t pos = 0;

    char str[1024];
    TEST_TRUE(length <= sizeof(str));
    memset(str, 'a', length);
    size_t depth = 2;

    size_t i;
    for (i = 0; i < depth; ++i) {
        mpack_build_array(&writer);
        expected[pos++] = '\x93';
        mpack_write_str(&writer, str, length);
        test_builder_add_expected_str(expected, &pos, str, length);
    }

    mpack_write_str(&writer, str, length);
    test_builder_add_expected_str(expected, &pos, str, length);

    for (i = 0; i < depth; ++i) {
        mpack_write_str(&writer, str, length);
        test_builder_add_expected_str(expected, &pos, str, length);
        mpack_complete_array(&writer);
    }

    TEST_TRUE(pos <= sizeof(expected));
    size_t used = mpack_writer_buffer_used(&writer);

    /*
    printf("actual %zi expected %zi\n", used, pos);
    for (i = 0; i < used; ++i) {
        printf("%02hhx ", buf[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    printf("\n");
    for (i = 0; i < pos; ++i) {
        printf("%02hhx ", expected[i]);
        if (((i+1) % 16)==0)
            printf("\n");
    }
    printf("\n");
    */

    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(used == pos);
    TEST_TRUE(0 == memcmp(buf, expected, used));
}

static void test_builder_strings(void) {
    test_builder_strings_length(3);
    test_builder_strings_length(17);
    test_builder_strings_length(32);
    test_builder_strings_length(129);
    test_builder_strings_length(457);
}

void test_builder(void) {
    test_builder_basic();
    test_builder_repeat();
    test_builder_nested();
    test_builder_deep();
    test_builder_large();
    test_builder_content();
    test_builder_strings();
}
#endif
