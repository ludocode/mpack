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

#include "test-node.h"

#if MPACK_NODE

mpack_error_t test_tree_error = mpack_ok;

void test_tree_error_handler(mpack_tree_t* tree, mpack_error_t error) {
    TEST_TRUE(test_tree_error == mpack_ok, "error handler was called multiple times");
    TEST_TRUE(error != mpack_ok, "error handler was called with mpack_ok");
    TEST_TRUE(mpack_tree_error(tree) == error, "tree error does not match given error");
    test_tree_error = error;
}

// tests the example on the messagepack homepage
static void test_example_node() {
    // add a junk byte at the end to test mpack_tree_size()
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00\xC1";
    mpack_tree_t tree;

    // this is a node pool test even if we have malloc. the rest of the
    // non-simple tests use paging unless malloc is unavailable.
    mpack_node_data_t pool[128];
    mpack_tree_init_pool(&tree, test, sizeof(test) - 1, pool, sizeof(pool) / sizeof(*pool));
    mpack_tree_parse(&tree);
    TEST_TRUE(mpack_tree_error(&tree) == mpack_ok);

    mpack_node_t map = mpack_tree_root(&tree);
    TEST_TRUE(true == mpack_node_bool(mpack_node_map_cstr(map, "compact")));
    TEST_TRUE(0 == mpack_node_u8(mpack_node_map_cstr(map, "schema")));
    TEST_TRUE(mpack_tree_size(&tree) == sizeof(test) - 2);

    TEST_TREE_DESTROY_NOERROR(&tree);
}

static void test_node_read_uint_fixnum() {
    mpack_node_data_t pool[128];

    // positive fixnums with u8
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_u8(node));

    // positive fixnums with u16
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_u16(node));

    // positive fixnums with u32
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_u32(node));

    // positive fixnums with u64
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_u64(node));

    // positive fixnums with uint
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_uint(node));

}

static void test_node_read_uint_signed_fixnum() {
    mpack_node_data_t pool[128];

    // positive fixnums with i8
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_i8(node));

    // positive fixnums with i16
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_i16(node));

    // positive fixnums with i32
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_i32(node));

    // positive fixnums with i64
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_i64(node));

    // positive fixnums with int
    TEST_SIMPLE_TREE_READ("\x00", 0 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\x01", 1 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\x02", 2 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\x0f", 0x0f == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\x10", 0x10 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\x7f", 0x7f == mpack_node_int(node));

}

static void test_node_read_negative_fixnum() {
    mpack_node_data_t pool[128];

    // negative fixnums with i8
    TEST_SIMPLE_TREE_READ("\xff", -1 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\xfe", -2 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\xf0", -16 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\xe0", -32 == mpack_node_i8(node));

    // negative fixnums with i16
    TEST_SIMPLE_TREE_READ("\xff", -1 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xfe", -2 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xf0", -16 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xe0", -32 == mpack_node_i16(node));

    // negative fixnums with i32
    TEST_SIMPLE_TREE_READ("\xff", -1 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xfe", -2 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xf0", -16 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xe0", -32 == mpack_node_i32(node));

    // negative fixnums with i64
    TEST_SIMPLE_TREE_READ("\xff", -1 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xfe", -2 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xf0", -16 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xe0", -32 == mpack_node_i64(node));

    // negative fixnums with int
    TEST_SIMPLE_TREE_READ("\xff", -1 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\xfe", -2 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\xf0", -16 == mpack_node_int(node));
    TEST_SIMPLE_TREE_READ("\xe0", -32 == mpack_node_int(node));

}

static void test_node_read_uint() {
    mpack_node_data_t pool[128];

    // positive signed into u8
    TEST_SIMPLE_TREE_READ("\xd0\x7f", 0x7f == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\xd0\x7f", 0x7f == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xd0\x7f", 0x7f == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xd0\x7f", 0x7f == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xd0\x7f", 0x7f == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\xd1\x7f\xff", 0x7fff == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xd1\x7f\xff", 0x7fff == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xd1\x7f\xff", 0x7fff == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xd1\x7f\xff", 0x7fff == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_node_uint(node));
    TEST_SIMPLE_TREE_READ("\xd3\x7f\xff\xff\xff\xff\xff\xff\xff", 0x7fffffffffffffff == mpack_node_u64(node));

    // positive unsigned into u8
    
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_u8(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_u16(node));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_u32(node));
    TEST_SIMPLE_TREE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_uint(node));

    TEST_SIMPLE_TREE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_node_u64(node));
    TEST_SIMPLE_TREE_READ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 0xffffffffffffffff == mpack_node_u64(node));

}

static void test_node_read_uint_signed() {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xcc\x80", 0x80 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xcc\xff", 0xff == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", 0x100 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", 0xffff == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_i64(node));

    TEST_SIMPLE_TREE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_node_i64(node));

}

static void test_node_read_int() {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ("\xd0\xdf", -33 == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\xd0\xdf", -33 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xd0\xdf", -33 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd0\xdf", -33 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd0\xdf", -33 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd0\x80", INT8_MIN == mpack_node_i8(node));
    TEST_SIMPLE_TREE_READ("\xd0\x80", INT8_MIN == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xd0\x80", INT8_MIN == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd0\x80", INT8_MIN == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd0\x80", INT8_MIN == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd1\x80\x00", INT16_MIN == mpack_node_i16(node));
    TEST_SIMPLE_TREE_READ("\xd1\x80\x00", INT16_MIN == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd1\x80\x00", INT16_MIN == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd1\x80\x00", INT16_MIN == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_node_i32(node));
    TEST_SIMPLE_TREE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_node_i64(node));
    TEST_SIMPLE_TREE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_node_int(node));

    TEST_SIMPLE_TREE_READ("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", (int64_t)INT32_MIN - 1 == mpack_node_i64(node));

    TEST_SIMPLE_TREE_READ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", INT64_MIN == mpack_node_i64(node));

}

static void test_node_read_ints_dynamic_int() {
    mpack_node_data_t pool[128];

    // we don't bother to test with different signed/unsigned value
    // functions; they are tested for equality in test-value.c

    // positive fixnums
    TEST_SIMPLE_TREE_READ("\x00", mpack_tag_equal(mpack_tag_uint(0), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\x01", mpack_tag_equal(mpack_tag_uint(1), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\x02", mpack_tag_equal(mpack_tag_uint(2), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\x0f", mpack_tag_equal(mpack_tag_uint(0x0f), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\x10", mpack_tag_equal(mpack_tag_uint(0x10), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\x7f", mpack_tag_equal(mpack_tag_uint(0x7f), mpack_node_tag(node)));

    // negative fixnums
    TEST_SIMPLE_TREE_READ("\xff", mpack_tag_equal(mpack_tag_int(-1), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xfe", mpack_tag_equal(mpack_tag_int(-2), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xf0", mpack_tag_equal(mpack_tag_int(-16), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xe0", mpack_tag_equal(mpack_tag_int(-32), mpack_node_tag(node)));

    // uints
    TEST_SIMPLE_TREE_READ("\xcc\x80", mpack_tag_equal(mpack_tag_uint(0x80), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xcc\xff", mpack_tag_equal(mpack_tag_uint(0xff), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xcd\x01\x00", mpack_tag_equal(mpack_tag_uint(0x100), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xcd\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffff), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xce\x00\x01\x00\x00", mpack_tag_equal(mpack_tag_uint(0x10000), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xce\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffffffff), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_uint(UINT64_C(0x100000000)), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(UINT64_C(0xffffffffffffffff)), mpack_node_tag(node)));

    // ints
    TEST_SIMPLE_TREE_READ("\xd0\xdf", mpack_tag_equal(mpack_tag_int(-33), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xd0\x80", mpack_tag_equal(mpack_tag_int(INT8_MIN), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xd1\xff\x7f", mpack_tag_equal(mpack_tag_int(INT8_MIN - 1), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xd1\x80\x00", mpack_tag_equal(mpack_tag_int(INT16_MIN), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xd2\xff\xff\x7f\xff", mpack_tag_equal(mpack_tag_int(INT16_MIN - 1), mpack_node_tag(node)));

    TEST_SIMPLE_TREE_READ("\xd2\x80\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT32_MIN), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_tag_equal(mpack_tag_int((int64_t)INT32_MIN - 1), mpack_node_tag(node)));

    TEST_SIMPLE_TREE_READ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT64_MIN), mpack_node_tag(node)));

}

static void test_node_read_int_bounds() {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ_ERROR("\xd1\xff\x7f", 0 == mpack_node_i8(node), mpack_error_type); 
    TEST_SIMPLE_TREE_READ_ERROR("\xd1\x80\x00", 0 == mpack_node_i8(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xd2\xff\xff\x7f\xff", 0 == mpack_node_i8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd2\xff\xff\x7f\xff", 0 == mpack_node_i16(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xd2\x80\x00\x00\x00", 0 == mpack_node_i8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd2\x80\x00\x00\x00", 0 == mpack_node_i16(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i16(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i32(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i16(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i32(node), mpack_error_type);

}

static void test_node_read_uint_bounds() {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ_ERROR("\xcd\x01\x00", 0 == mpack_node_u8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xcd\xff\xff", 0 == mpack_node_u8(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xce\x00\x01\x00\x00", 0 == mpack_node_u8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xce\x00\x01\x00\x00", 0 == mpack_node_u16(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xce\xff\xff\xff\xff", 0 == mpack_node_u8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xce\xff\xff\xff\xff", 0 == mpack_node_u16(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u16(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u32(node), mpack_error_type);

}

static void test_node_read_misc() {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ("\xc0", (mpack_node_nil(node), true));

    TEST_SIMPLE_TREE_READ("\xc2", false == mpack_node_bool(node));
    TEST_SIMPLE_TREE_READ("\xc2", (mpack_node_false(node), true));
    TEST_SIMPLE_TREE_READ("\xc3", true == mpack_node_bool(node));
    TEST_SIMPLE_TREE_READ("\xc3", (mpack_node_true(node), true));

    TEST_SIMPLE_TREE_READ_ERROR("\xc2", (mpack_node_true(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc3", (mpack_node_false(node), true), mpack_error_type);

    TEST_SIMPLE_TREE_READ("\xc0", mpack_tag_equal(mpack_tag_nil(), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xc2", mpack_tag_equal(mpack_tag_false(), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xc3", mpack_tag_equal(mpack_tag_true(), mpack_node_tag(node)));

    // test missing space for cstr null-terminator
    mpack_tree_t tree;
    mpack_tree_init_pool(&tree, "\xa0", 1, pool, sizeof(pool) / sizeof(*pool));
    mpack_tree_parse(&tree);
    #if MPACK_DEBUG
    char buf[1];
    TEST_ASSERT(mpack_node_copy_cstr(mpack_tree_root(&tree), buf, 0));
    #endif
    #ifdef MPACK_MALLOC
    TEST_BREAK(NULL == mpack_node_cstr_alloc(mpack_tree_root(&tree), 0));
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_bug);
    #else
    TEST_TREE_DESTROY_NOERROR(&tree);
    #endif

    // test pool too small
    mpack_node_data_t small_pool[1];
    mpack_tree_init_pool(&tree, "\x91\xc0", 2, small_pool, 1);
    mpack_tree_parse(&tree);
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_too_big);
    TEST_BREAK((mpack_tree_init_pool(&tree, "\xc0", 1, small_pool, 0), true));
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_bug);
}

static void test_node_read_floats() {
    mpack_node_data_t pool[128];

    // these are some very simple floats that don't really test IEEE 742 conformance;
    // this section could use some improvement

    TEST_SIMPLE_TREE_READ("\x00", 0.0f == mpack_node_float(node));
    TEST_SIMPLE_TREE_READ("\xd0\x00", 0.0f == mpack_node_float(node));
    TEST_SIMPLE_TREE_READ("\xca\x00\x00\x00\x00", 0.0f == mpack_node_float(node));
    TEST_SIMPLE_TREE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_node_float(node));

    TEST_SIMPLE_TREE_READ("\x00", 0.0 == mpack_node_double(node));
    TEST_SIMPLE_TREE_READ("\xd0\x00", 0.0 == mpack_node_double(node));
    TEST_SIMPLE_TREE_READ("\xca\x00\x00\x00\x00", 0.0 == mpack_node_double(node));
    TEST_SIMPLE_TREE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0 == mpack_node_double(node));

    TEST_SIMPLE_TREE_READ("\xca\x00\x00\x00\x00", 0.0f == mpack_node_float_strict(node));
    TEST_SIMPLE_TREE_READ("\xca\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_float(0.0f), mpack_node_tag(node)));
    TEST_SIMPLE_TREE_READ("\xca\x00\x00\x00\x00", 0.0 == mpack_node_double_strict(node));
    TEST_SIMPLE_TREE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_double(0.0), mpack_node_tag(node)));

    // when -ffinite-math-only is enabled, isnan() can always return false.
    // TODO: we should probably add at least a reader option to
    // generate an error on non-finite reals.
    #if !MPACK_FINITE_MATH
    TEST_SIMPLE_TREE_READ("\xca\xff\xff\xff\xff", isnanf(mpack_node_float(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnanf(mpack_node_float(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xca\xff\xff\xff\xff", isnan(mpack_node_double(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_node_double(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xca\xff\xff\xff\xff", isnanf(mpack_node_float_strict(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xca\xff\xff\xff\xff", isnan(mpack_node_double_strict(node)) != 0);
    TEST_SIMPLE_TREE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_node_double_strict(node)) != 0);
    #endif

    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd0\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0.0 == mpack_node_double_strict(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xd0\x00", 0.0 == mpack_node_double_strict(node), mpack_error_type);
}

static void test_node_read_bad_type() {
    mpack_node_data_t pool[128];

    // test that non-compound node functions correctly handle badly typed data
    TEST_SIMPLE_TREE_READ_ERROR("\xc2", (mpack_node_nil(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", false == mpack_node_bool(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_u8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_u16(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_u32(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_u64(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_uint(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_i8(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_i16(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_i32(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_i64(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_int(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0.0f == mpack_node_float(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0.0 == mpack_node_double(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0.0 == mpack_node_double_strict(node), mpack_error_type);
}

static void test_node_read_possible() {
    // test early exit for data that contains impossible node numbers

    mpack_node_data_t pool[128];
    TEST_SIMPLE_TREE_READ_ERROR("\xcc", (MPACK_UNUSED(node), true), mpack_error_invalid); // truncated u8
    TEST_SIMPLE_TREE_READ_ERROR("\xcd", (MPACK_UNUSED(node), true), mpack_error_invalid); // truncated u16
    TEST_SIMPLE_TREE_READ_ERROR("\xce", (MPACK_UNUSED(node), true), mpack_error_invalid); // truncated u32
    TEST_SIMPLE_TREE_READ_ERROR("\xcf", (MPACK_UNUSED(node), true), mpack_error_invalid); // truncated u64

    #ifdef MPACK_MALLOC
    // this is an example of a potential denial-of-service attack against
    // MessagePack implementations that allocate storage up-front. this
    // should be handled safely without allocating huge amounts of memory.
    const char* attack =
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff"
            "\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff\xdd\xff\xff\xff\xff";
    size_t allocation_count = test_malloc_total_count();
    mpack_tree_t tree;
    mpack_tree_init(&tree, attack, strlen(attack));
    mpack_tree_parse(&tree);
    allocation_count = test_malloc_total_count() - allocation_count;
    TEST_TRUE(allocation_count <= 2, "too many allocations! %i calls to malloc()", (int)allocation_count);
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_invalid);
    #endif
}

static void test_node_read_pre_error() {
    mpack_node_data_t pool[128];
    char buf[1];

    // test that all node functions correctly handle pre-existing errors

    TEST_SIMPLE_TREE_READ_ERROR("\xc1", mpack_type_nil == mpack_node_tag(node).type, mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", (mpack_node_nil(node), true), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", false == mpack_node_bool(node), mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_u8(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_u16(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_u32(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_u64(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_uint(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_i8(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_i16(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_i32(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_i64(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_int(node), mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", 0.0f == mpack_node_float(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0.0 == mpack_node_double(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0.0f == mpack_node_float_strict(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0.0 == mpack_node_double_strict(node), mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_array_length(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_array_at(node, 0).data, mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_map_count(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_key_at(node, 0).data, mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_value_at(node, 0).data, mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_uint(node, 1).data, mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_int(node, -1).data, mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_str(node, "test", 4).data, mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", &tree.nil_node == mpack_node_map_cstr(node, "test").data, mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", false == mpack_node_map_contains_str(node, "test", 4), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", false == mpack_node_map_contains_cstr(node, "test"), mpack_error_invalid);

    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_exttype(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_data_len(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_strlen(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", NULL == mpack_node_data(node), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", 0 == mpack_node_copy_data(node, NULL, 0), mpack_error_invalid);
    buf[0] = 1;
    TEST_SIMPLE_TREE_READ_ERROR("", (mpack_node_copy_cstr(node, buf, sizeof(buf)), true), mpack_error_invalid);
    TEST_TRUE(buf[0] == 0);
    #ifdef MPACK_MALLOC
    TEST_SIMPLE_TREE_READ_ERROR("", NULL == mpack_node_data_alloc(node, 0), mpack_error_invalid);
    TEST_SIMPLE_TREE_READ_ERROR("", NULL == mpack_node_cstr_alloc(node, 0), mpack_error_invalid);
    #endif
}

static void test_node_read_strings() {
    char buf[256];
    mpack_node_data_t pool[128];
    #ifdef MPACK_MALLOC
    char* test = NULL;
    #endif

    // these are copied from test-expect.c
    // the first byte of each of these is the MessagePack object header
    const char utf8_null[]            = "\xa1\x00";
    const char utf8_valid[]           = "\xac \xCF\x80 \xe4\xb8\xad \xf0\xa0\x80\xb6";
    const char utf8_trimmed[]         = "\xa4\xf0\xa0\x80\xb6";
    const char utf8_invalid[]         = "\xa3 \x80 ";
    const char utf8_invalid_trimmed[] = "\xa1\xa0";
    const char utf8_truncated[]       = "\xa2\xf0\xa0";
    // we don't accept any of these UTF-8 variants; only pure UTF-8 is allowed.
    const char utf8_modified[]        = "\xa4 \xc0\x80 ";
    const char utf8_cesu8[]           = "\xa8 \xED\xA0\x81\xED\xB0\x80 ";
    const char utf8_wobbly[]          = "\xa5 \xED\xA0\x81 ";

    // utf8 str check
    TEST_SIMPLE_TREE_READ("\xa0", (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ("\xa0", (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ("\xa4test", (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ(utf8_null, (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ(utf8_valid, (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ(utf8_trimmed, (mpack_node_check_utf8(node), true));
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid, (mpack_node_check_utf8(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid_trimmed, (mpack_node_check_utf8(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_truncated, (mpack_node_check_utf8(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_modified, (mpack_node_check_utf8(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_cesu8, (mpack_node_check_utf8(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_wobbly, (mpack_node_check_utf8(node), true), mpack_error_type);

    // utf8 cstr check
    TEST_SIMPLE_TREE_READ("\xa0", (mpack_node_check_utf8_cstr(node), true));
    TEST_SIMPLE_TREE_READ("\xa4test", (mpack_node_check_utf8_cstr(node), true));
    TEST_SIMPLE_TREE_READ_ERROR(utf8_null, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ(utf8_valid, (mpack_node_check_utf8_cstr(node), true));
    TEST_SIMPLE_TREE_READ(utf8_trimmed, (mpack_node_check_utf8_cstr(node), true));
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid_trimmed, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_truncated, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_modified, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_cesu8, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_wobbly, (mpack_node_check_utf8_cstr(node), true), mpack_error_type);

    // utf8 str copy
    TEST_SIMPLE_TREE_READ("\xa0", 0 == mpack_node_copy_utf8(node, buf, 0));
    TEST_SIMPLE_TREE_READ("\xa0", 0 == mpack_node_copy_utf8(node, buf, 4));
    TEST_SIMPLE_TREE_READ("\xa4test", 4 == mpack_node_copy_utf8(node, buf, 4));
    TEST_SIMPLE_TREE_READ_ERROR("\xa5hello", 0 == mpack_node_copy_utf8(node, buf, 4), mpack_error_too_big);
    TEST_SIMPLE_TREE_READ_ERROR("\xc0", 0 == mpack_node_copy_utf8(node, buf, 4), mpack_error_type);
    TEST_SIMPLE_TREE_READ(utf8_null, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_valid, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_trimmed, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid_trimmed, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_truncated, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_modified, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_cesu8, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_wobbly, (mpack_node_copy_utf8(node, buf, sizeof(buf)), true), mpack_error_type);

    // cstr copy
    TEST_SIMPLE_TREE_READ_ASSERT("\xa0", mpack_node_copy_cstr(node, buf, 0));
    TEST_SIMPLE_TREE_READ("\xa0", (mpack_node_copy_cstr(node, buf, 4), true));
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_TREE_READ("\xa4test", (mpack_node_copy_cstr(node, buf, 5), true));
    TEST_TRUE(strlen(buf) == 4);
    TEST_SIMPLE_TREE_READ_ERROR("\xa5hello", (mpack_node_copy_cstr(node, buf, 5), true), mpack_error_too_big);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_null, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_TREE_READ(utf8_valid, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_invalid, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_invalid_trimmed, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_truncated, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_modified, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_cesu8, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_wobbly, (mpack_node_copy_cstr(node, buf, sizeof(buf)), true));

    // utf8 cstr copy
    TEST_SIMPLE_TREE_READ_ASSERT("\xa0", mpack_node_copy_utf8_cstr(node, buf, 0));
    TEST_SIMPLE_TREE_READ("\xa0", (mpack_node_copy_utf8_cstr(node, buf, 4), true));
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_TREE_READ("\xa4test", (mpack_node_copy_utf8_cstr(node, buf, 5), true));
    TEST_TRUE(strlen(buf) == 4);
    TEST_SIMPLE_TREE_READ_ERROR("\xa5hello", (mpack_node_copy_utf8_cstr(node, buf, 5), true), mpack_error_too_big);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_null, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ(utf8_valid, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ(utf8_trimmed, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true));
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid_trimmed, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_truncated, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_modified, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_cesu8, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_wobbly, (mpack_node_copy_utf8_cstr(node, buf, sizeof(buf)), true), mpack_error_type);

    #ifdef MPACK_MALLOC
    // cstr alloc
    TEST_SIMPLE_TREE_READ_ERROR(utf8_null, NULL == mpack_node_cstr_alloc(node, 256), mpack_error_type);

    // utf8 cstr alloc
    TEST_SIMPLE_TREE_READ_BREAK("\xa0", NULL == mpack_node_utf8_cstr_alloc(node, 0));
    TEST_SIMPLE_TREE_READ("\xa0", NULL != (test = mpack_node_utf8_cstr_alloc(node, 4)));
    if (test) {
        TEST_TRUE(strlen(test) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_TREE_READ_ERROR("\xa4test", NULL == mpack_node_utf8_cstr_alloc(node, 4), mpack_error_too_big);
    TEST_SIMPLE_TREE_READ("\xa4test", NULL != (test = mpack_node_utf8_cstr_alloc(node, 5)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_TREE_READ("\xa4test", NULL != (test = mpack_node_utf8_cstr_alloc(node, SIZE_MAX)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_TREE_READ_ERROR("\x01", NULL == mpack_node_utf8_cstr_alloc(node, 3), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR(utf8_null, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ(utf8_valid, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true));
    MPACK_FREE(test);
    TEST_SIMPLE_TREE_READ(utf8_trimmed, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true));
    MPACK_FREE(test);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_invalid_trimmed, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_truncated, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_modified, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_cesu8, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR(utf8_wobbly, ((test = mpack_node_utf8_cstr_alloc(node, 256)), true), mpack_error_type);
    #endif
}

static void test_node_read_enum() {
    mpack_node_data_t pool[128];

    typedef enum           { APPLE ,  BANANA ,  ORANGE , COUNT} fruit_t;
    const char* fruits[] = {"apple", "banana", "orange"};

    TEST_SIMPLE_TREE_READ("\xa5""apple", APPLE == (fruit_t)mpack_node_enum(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\xa6""banana", BANANA == (fruit_t)mpack_node_enum(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\xa6""orange", ORANGE == (fruit_t)mpack_node_enum(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ_ERROR("\xa4""kiwi", COUNT == (fruit_t)mpack_node_enum(node, fruits, COUNT), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x01", COUNT == (fruit_t)mpack_node_enum(node, fruits, COUNT), mpack_error_type);

    TEST_SIMPLE_TREE_READ("\xa5""apple", APPLE == (fruit_t)mpack_node_enum_optional(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\xa6""banana", BANANA == (fruit_t)mpack_node_enum_optional(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\xa6""orange", ORANGE == (fruit_t)mpack_node_enum_optional(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\xa4""kiwi", COUNT == (fruit_t)mpack_node_enum_optional(node, fruits, COUNT));
    TEST_SIMPLE_TREE_READ("\x01", COUNT == (fruit_t)mpack_node_enum_optional(node, fruits, COUNT));

    // test pre-existing error
    TEST_SIMPLE_TREE_READ_ERROR("\x01", (mpack_node_nil(node), COUNT == (fruit_t)mpack_node_enum(node, fruits, COUNT)), mpack_error_type);
}

static void test_node_read_array() {
    static const char test[] = "\x93\x90\x91\xc3\x92\xc3\xc3";
    mpack_tree_t tree;
    TEST_TREE_INIT(&tree, test, sizeof(test) - 1);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    TEST_TRUE(mpack_type_array == mpack_node_type(root));
    TEST_TRUE(3 == mpack_node_array_length(root));

    TEST_TRUE(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 0)));
    TEST_TRUE(0 == mpack_node_array_length(mpack_node_array_at(root, 0)));

    TEST_TRUE(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 1)));
    TEST_TRUE(1 == mpack_node_array_length(mpack_node_array_at(root, 1)));
    TEST_TRUE(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 1), 0)));
    TEST_TRUE(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 1), 0)));

    TEST_TRUE(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 2)));
    TEST_TRUE(2 == mpack_node_array_length(mpack_node_array_at(root, 2)));
    TEST_TRUE(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 2), 0)));
    TEST_TRUE(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 2), 0)));
    TEST_TRUE(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 2), 1)));
    TEST_TRUE(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 2), 1)));

    TEST_TRUE(mpack_ok == mpack_tree_error(&tree));

    // test out of bounds
    TEST_TRUE(mpack_type_nil == mpack_node_type(mpack_node_array_at(root, 4)));
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_data);
}

static void test_node_read_map() {
    // test map using maps as keys and values
    static const char test[] = "\x82\x80\x81\x01\x02\x81\x03\x04\xc3";
    mpack_tree_t tree;
    TEST_TREE_INIT(&tree, test, sizeof(test) - 1);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    TEST_TRUE(mpack_type_map == mpack_node_type(root));
    TEST_TRUE(2 == mpack_node_map_count(root));

    TEST_TRUE(mpack_type_map == mpack_node_type(mpack_node_map_key_at(root, 0)));
    TEST_TRUE(0 == mpack_node_map_count(mpack_node_map_key_at(root, 0)));

    TEST_TRUE(mpack_type_map == mpack_node_type(mpack_node_map_value_at(root, 0)));
    TEST_TRUE(1 == mpack_node_map_count(mpack_node_map_value_at(root, 0)));
    TEST_TRUE(1 == mpack_node_i32(mpack_node_map_key_at(mpack_node_map_value_at(root, 0), 0)));
    TEST_TRUE(2 == mpack_node_i32(mpack_node_map_value_at(mpack_node_map_value_at(root, 0), 0)));

    TEST_TRUE(mpack_type_map == mpack_node_type(mpack_node_map_key_at(root, 1)));
    TEST_TRUE(1 == mpack_node_map_count(mpack_node_map_key_at(root, 1)));
    TEST_TRUE(3 == mpack_node_i32(mpack_node_map_key_at(mpack_node_map_key_at(root, 1), 0)));
    TEST_TRUE(4 == mpack_node_i32(mpack_node_map_value_at(mpack_node_map_key_at(root, 1), 0)));

    TEST_TRUE(mpack_type_bool == mpack_node_type(mpack_node_map_value_at(root, 1)));
    TEST_TRUE(true == mpack_node_bool(mpack_node_map_value_at(root, 1)));

    TEST_TRUE(mpack_ok == mpack_tree_error(&tree));

    // test out of bounds
    TEST_TRUE(mpack_type_nil == mpack_node_type(mpack_node_map_key_at(root, 2)));
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_data);
}

static void test_node_read_map_search() {
    static const char test[] =
            "\x89\x00\x01\xd0\x7f\x02\xfe\x03\xa5""alice\x04\xa3"
            "bob\x05\xa4""carl\x06\xa4""carl\x07\x10\x08\x10\x09";

    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ(test, 1 == mpack_node_i32(mpack_node_map_uint(node, 0)));
    TEST_SIMPLE_TREE_READ(test, 1 == mpack_node_i32(mpack_node_map_int(node, 0)));
    TEST_SIMPLE_TREE_READ(test, 2 == mpack_node_i32(mpack_node_map_uint(node, 127))); // underlying tag type is int
    TEST_SIMPLE_TREE_READ(test, 3 == mpack_node_i32(mpack_node_map_int(node, -2)));
    TEST_SIMPLE_TREE_READ(test, 4 == mpack_node_i32(mpack_node_map_str(node, "alice", 5)));
    TEST_SIMPLE_TREE_READ(test, 5 == mpack_node_i32(mpack_node_map_cstr(node, "bob")));

    TEST_SIMPLE_TREE_READ(test, mpack_node_map_contains_int(node, 0));
    TEST_SIMPLE_TREE_READ(test, mpack_node_map_contains_uint(node, 0));
    TEST_SIMPLE_TREE_READ(test, false == mpack_node_map_contains_int(node, 1));
    TEST_SIMPLE_TREE_READ(test, false == mpack_node_map_contains_uint(node, 1));
    TEST_SIMPLE_TREE_READ(test, mpack_node_map_contains_int(node, -2));
    TEST_SIMPLE_TREE_READ(test, false == mpack_node_map_contains_int(node, -3));

    TEST_SIMPLE_TREE_READ(test, true == mpack_node_map_contains_str(node, "alice", 5));
    TEST_SIMPLE_TREE_READ(test, true == mpack_node_map_contains_cstr(node, "bob"));
    TEST_SIMPLE_TREE_READ(test, false == mpack_node_map_contains_str(node, "eve", 3));
    TEST_SIMPLE_TREE_READ(test, false == mpack_node_map_contains_cstr(node, "eve"));

    TEST_SIMPLE_TREE_READ_ERROR(test, false == mpack_node_map_contains_int(node, 16), mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR(test, false == mpack_node_map_contains_uint(node, 16), mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR(test, false == mpack_node_map_contains_str(node, "carl", 4), mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR(test, false == mpack_node_map_contains_cstr(node, "carl"), mpack_error_data);
}

static void test_node_read_compound_errors(void) {
    mpack_node_data_t pool[128];

    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_array_length(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_map_count(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_array_at(node, 0).data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_key_at(node, 0).data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_value_at(node, 0).data, mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\x80", &tree.nil_node == mpack_node_map_int(node, -1).data, mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR("\x80", &tree.nil_node == mpack_node_map_uint(node, 1).data, mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR("\x80", &tree.nil_node == mpack_node_map_str(node, "test", 4).data, mpack_error_data);
    TEST_SIMPLE_TREE_READ_ERROR("\x80", &tree.nil_node == mpack_node_map_cstr(node, "test").data, mpack_error_data);

    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_int(node, -1).data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_uint(node, 1).data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_str(node, "test", 4).data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", &tree.nil_node == mpack_node_map_cstr(node, "test").data, mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", false == mpack_node_map_contains_str(node, "test", 4), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", false == mpack_node_map_contains_cstr(node, "test"), mpack_error_type);

    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_exttype(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_data_len(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_strlen(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", NULL == mpack_node_data(node), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", 0 == mpack_node_copy_data(node, NULL, 0), mpack_error_type);

    char data[1] = {'a'};
    TEST_SIMPLE_TREE_READ_ERROR("\x00", (mpack_node_copy_data(node, data, 1), true), mpack_error_type);
    TEST_TRUE(data[0] == 'a');
    TEST_SIMPLE_TREE_READ_ERROR("\x00", (mpack_node_copy_cstr(node, data, 1), true), mpack_error_type);
    TEST_TRUE(data[0] == 0);

    #ifdef MPACK_MALLOC
    TEST_SIMPLE_TREE_READ_ERROR("\x00", NULL == mpack_node_data_alloc(node, 10), mpack_error_type);
    TEST_SIMPLE_TREE_READ_ERROR("\x00", NULL == mpack_node_cstr_alloc(node, 10), mpack_error_type);
    #endif

    data[0] = 'a';
    TEST_SIMPLE_TREE_READ_ERROR("\xa3""bob", (mpack_node_copy_data(node, data, 2), true), mpack_error_too_big);
    TEST_TRUE(data[0] == 'a');
    TEST_SIMPLE_TREE_READ_ERROR("\xa3""bob", (mpack_node_copy_cstr(node, data, 2), true), mpack_error_too_big);
    TEST_TRUE(data[0] == 0);

    #ifdef MPACK_MALLOC
    TEST_SIMPLE_TREE_READ_ERROR("\xa3""bob", NULL == mpack_node_cstr_alloc(node, 2), mpack_error_too_big);
    TEST_SIMPLE_TREE_READ_ERROR("\xa3""bob", NULL == mpack_node_data_alloc(node, 2), mpack_error_too_big);
    #endif
}

static void test_node_read_data(void) {
    static const char test[] = "\x93\xa5""alice\xc4\x03""bob\xd6\x07""carl";
    mpack_tree_t tree;
    TEST_TREE_INIT(&tree, test, sizeof(test) - 1);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    mpack_node_t alice = mpack_node_array_at(root, 0);
    TEST_TRUE(5 == mpack_node_data_len(alice));
    TEST_TRUE(5 == mpack_node_strlen(alice));
    TEST_TRUE(NULL != mpack_node_data(alice));
    TEST_TRUE(0 == memcmp("alice", mpack_node_data(alice), 5));

    char alice_data[6] = {'s','s','s','s','s','s'};
    mpack_node_copy_data(alice, alice_data, sizeof(alice_data));
    TEST_TRUE(0 == memcmp("alices", alice_data, 6));
    mpack_node_copy_cstr(alice, alice_data, sizeof(alice_data));
    TEST_TRUE(0 == strcmp("alice", alice_data));

    #ifdef MPACK_MALLOC
    char* alice_alloc = mpack_node_cstr_alloc(alice, 100);
    TEST_TRUE(0 == strcmp("alice", alice_alloc));
    MPACK_FREE(alice_alloc);
    #endif

    mpack_node_t bob = mpack_node_array_at(root, 1);
    TEST_TRUE(3 == mpack_node_data_len(bob));
    TEST_TRUE(0 == memcmp("bob", mpack_node_data(bob), 3));

    #ifdef MPACK_MALLOC
    char* bob_alloc = mpack_node_data_alloc(bob, 100);
    TEST_TRUE(0 == memcmp("bob", bob_alloc, 3));
    MPACK_FREE(bob_alloc);
    #endif

    mpack_node_t carl = mpack_node_array_at(root, 2);
    TEST_TRUE(7 == mpack_node_exttype(carl));
    TEST_TRUE(4 == mpack_node_data_len(carl));
    TEST_TRUE(0 == memcmp("carl", mpack_node_data(carl), 4));

    TEST_TREE_DESTROY_NOERROR(&tree);
}

static void test_node_read_deep_stack(void) {
    static const int depth = 1200;
    char buf[4096];

    uint8_t* p = (uint8_t*)buf;
    for (int i = 0; i < depth; ++i) {
        *p++ = 0x81; // one pair map
        *p++ = 0x04; // key four
        *p++ = 0x91; // value one element array
    }
    *p++ = 0x07; // final array value seven

    mpack_tree_t tree;
    TEST_TREE_INIT(&tree, buf, (size_t)(p - (uint8_t*)buf));
    mpack_tree_parse(&tree);

    #ifdef MPACK_MALLOC
    mpack_node_t node = mpack_tree_root(&tree);
    for (int i = 0; i < depth; ++i) {
        TEST_TRUE(mpack_tree_error(&tree) == mpack_ok, "error at depth %i", i);
        TEST_TRUE(mpack_node_map_count(node) == 1, "error at depth %i", i);
        TEST_TRUE(mpack_node_u8(mpack_node_map_key_at(node, 0)) == 4, "error at depth %i", i);
        TEST_TRUE(mpack_node_array_length(mpack_node_map_value_at(node, 0)) == 1, "error at depth %i", i);
        node = mpack_node_array_at(mpack_node_map_value_at(node, 0), 0);
    }
    TEST_TRUE(mpack_node_u8(node) == 7, "error in final node");
    TEST_TREE_DESTROY_NOERROR(&tree);
    #else
    TEST_TREE_DESTROY_ERROR(&tree, mpack_error_too_big);
    #endif
}

void test_node(void) {
    test_example_node();

    // int/uint
    test_node_read_uint_fixnum();
    test_node_read_uint_signed_fixnum();
    test_node_read_negative_fixnum();
    test_node_read_uint();
    test_node_read_uint_signed();
    test_node_read_int();
    test_node_read_uint_bounds();
    test_node_read_int_bounds();
    test_node_read_ints_dynamic_int();

    // other
    test_node_read_misc();
    test_node_read_floats();
    test_node_read_bad_type();
    test_node_read_possible();
    test_node_read_pre_error();
    test_node_read_strings();
    test_node_read_enum();

    // compound types
    test_node_read_array();
    test_node_read_map();
    test_node_read_map_search();
    test_node_read_compound_errors();
    test_node_read_data();
    test_node_read_deep_stack();
}

#endif

