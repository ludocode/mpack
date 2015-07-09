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

#include "test-node.h"

#if MPACK_NODE

// tests the example on the messagepack homepage
static void test_example_node() {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";
    mpack_tree_t tree;

    // this is a node pool test even if we have malloc. the rest of the
    // non-simple tests use paging unless malloc is unavailable.
    mpack_node_t nodes[128];
    mpack_tree_init_pool(&tree, test, sizeof(test) - 1, nodes, sizeof(nodes) / sizeof(*nodes));
    //mpack_node_print(mpack_tree_root(&tree));

    #if MPACK_SETJMP
    if (MPACK_TREE_SETJMP(&tree)) {
        test_assert(0, "jumped! error: %s", mpack_error_to_string(mpack_tree_error(&tree)));
        return;
    }
    #endif

    mpack_node_t* map = mpack_tree_root(&tree);
    test_assert(true == mpack_node_bool(mpack_node_map_cstr(map, "compact")));
    test_assert(0 == mpack_node_u8(mpack_node_map_cstr(map, "schema")));

    test_tree_destroy_noerror(&tree);
}

static void test_node_read_uint_fixnum() {
    mpack_node_t nodes[128];

    // positive fixnums with u8
    test_simple_tree_read("\x00", 0 == mpack_node_u8(node));
    test_simple_tree_read("\x01", 1 == mpack_node_u8(node));
    test_simple_tree_read("\x02", 2 == mpack_node_u8(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_u8(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_u8(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_u8(node));

    // positive fixnums with u16
    test_simple_tree_read("\x00", 0 == mpack_node_u16(node));
    test_simple_tree_read("\x01", 1 == mpack_node_u16(node));
    test_simple_tree_read("\x02", 2 == mpack_node_u16(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_u16(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_u16(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_u16(node));

    // positive fixnums with u32
    test_simple_tree_read("\x00", 0 == mpack_node_u32(node));
    test_simple_tree_read("\x01", 1 == mpack_node_u32(node));
    test_simple_tree_read("\x02", 2 == mpack_node_u32(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_u32(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_u32(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_u32(node));

    // positive fixnums with u64
    test_simple_tree_read("\x00", 0 == mpack_node_u64(node));
    test_simple_tree_read("\x01", 1 == mpack_node_u64(node));
    test_simple_tree_read("\x02", 2 == mpack_node_u64(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_u64(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_u64(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_u64(node));

}

static void test_node_read_uint_signed_fixnum() {
    mpack_node_t nodes[128];

    // positive fixnums with i8
    test_simple_tree_read("\x00", 0 == mpack_node_i8(node));
    test_simple_tree_read("\x01", 1 == mpack_node_i8(node));
    test_simple_tree_read("\x02", 2 == mpack_node_i8(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_i8(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_i8(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_i8(node));

    // positive fixnums with i16
    test_simple_tree_read("\x00", 0 == mpack_node_i16(node));
    test_simple_tree_read("\x01", 1 == mpack_node_i16(node));
    test_simple_tree_read("\x02", 2 == mpack_node_i16(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_i16(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_i16(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_i16(node));

    // positive fixnums with i32
    test_simple_tree_read("\x00", 0 == mpack_node_i32(node));
    test_simple_tree_read("\x01", 1 == mpack_node_i32(node));
    test_simple_tree_read("\x02", 2 == mpack_node_i32(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_i32(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_i32(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_i32(node));

    // positive fixnums with i64
    test_simple_tree_read("\x00", 0 == mpack_node_i64(node));
    test_simple_tree_read("\x01", 1 == mpack_node_i64(node));
    test_simple_tree_read("\x02", 2 == mpack_node_i64(node));
    test_simple_tree_read("\x0f", 0x0f == mpack_node_i64(node));
    test_simple_tree_read("\x10", 0x10 == mpack_node_i64(node));
    test_simple_tree_read("\x7f", 0x7f == mpack_node_i64(node));

}

static void test_node_read_negative_fixnum() {
    mpack_node_t nodes[128];

    // negative fixnums with i8
    test_simple_tree_read("\xff", -1 == mpack_node_i8(node));
    test_simple_tree_read("\xfe", -2 == mpack_node_i8(node));
    test_simple_tree_read("\xf0", -16 == mpack_node_i8(node));
    test_simple_tree_read("\xe0", -32 == mpack_node_i8(node));

    // negative fixnums with i16
    test_simple_tree_read("\xff", -1 == mpack_node_i16(node));
    test_simple_tree_read("\xfe", -2 == mpack_node_i16(node));
    test_simple_tree_read("\xf0", -16 == mpack_node_i16(node));
    test_simple_tree_read("\xe0", -32 == mpack_node_i16(node));

    // negative fixnums with i32
    test_simple_tree_read("\xff", -1 == mpack_node_i32(node));
    test_simple_tree_read("\xfe", -2 == mpack_node_i32(node));
    test_simple_tree_read("\xf0", -16 == mpack_node_i32(node));
    test_simple_tree_read("\xe0", -32 == mpack_node_i32(node));

    // negative fixnums with i64
    test_simple_tree_read("\xff", -1 == mpack_node_i64(node));
    test_simple_tree_read("\xfe", -2 == mpack_node_i64(node));
    test_simple_tree_read("\xf0", -16 == mpack_node_i64(node));
    test_simple_tree_read("\xe0", -32 == mpack_node_i64(node));

}

static void test_node_read_uint() {
    mpack_node_t nodes[128];

    // positive signed into u8
    test_simple_tree_read("\xd0\x7f", 0x7f == mpack_node_u8(node));
    test_simple_tree_read("\xd0\x7f", 0x7f == mpack_node_u16(node));
    test_simple_tree_read("\xd0\x7f", 0x7f == mpack_node_u32(node));
    test_simple_tree_read("\xd0\x7f", 0x7f == mpack_node_u64(node));
    test_simple_tree_read("\xd1\x7f\xff", 0x7fff == mpack_node_u16(node));
    test_simple_tree_read("\xd1\x7f\xff", 0x7fff == mpack_node_u32(node));
    test_simple_tree_read("\xd1\x7f\xff", 0x7fff == mpack_node_u64(node));
    test_simple_tree_read("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_node_u32(node));
    test_simple_tree_read("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_node_u64(node));
    test_simple_tree_read("\xd3\x7f\xff\xff\xff\xff\xff\xff\xff", 0x7fffffffffffffff == mpack_node_u64(node));

    // positive unsigned into u8
    
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_u8(node));
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_u16(node));
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_u32(node));
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_u64(node));

    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_u8(node));
    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_u16(node));
    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_u32(node));
    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_u64(node));

    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_u16(node));
    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_u32(node));
    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_u64(node));

    test_simple_tree_read("\xcd\xff\xff", 0xffff == mpack_node_u16(node));
    test_simple_tree_read("\xcd\xff\xff", 0xffff == mpack_node_u32(node));
    test_simple_tree_read("\xcd\xff\xff", 0xffff == mpack_node_u64(node));

    test_simple_tree_read("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_u32(node));
    test_simple_tree_read("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_u64(node));

    test_simple_tree_read("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_u32(node));
    test_simple_tree_read("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_u64(node));

    test_simple_tree_read("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_node_u64(node));
    test_simple_tree_read("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 0xffffffffffffffff == mpack_node_u64(node));

}

static void test_node_read_uint_signed() {
    mpack_node_t nodes[128];

    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_i16(node));
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_i32(node));
    test_simple_tree_read("\xcc\x80", 0x80 == mpack_node_i64(node));

    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_i16(node));
    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_i32(node));
    test_simple_tree_read("\xcc\xff", 0xff == mpack_node_i64(node));

    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_i16(node));
    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_i32(node));
    test_simple_tree_read("\xcd\x01\x00", 0x100 == mpack_node_i64(node));

    test_simple_tree_read("\xcd\xff\xff", 0xffff == mpack_node_i32(node));
    test_simple_tree_read("\xcd\xff\xff", 0xffff == mpack_node_i64(node));

    test_simple_tree_read("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_i32(node));
    test_simple_tree_read("\xce\x00\x01\x00\x00", 0x10000 == mpack_node_i64(node));

    test_simple_tree_read("\xce\xff\xff\xff\xff", 0xffffffff == mpack_node_i64(node));

    test_simple_tree_read("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_node_i64(node));

}

static void test_node_read_int() {
    mpack_node_t nodes[128];

    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i8(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i16(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i32(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i64(node));

    test_simple_tree_read("\xd0\x80", INT8_MIN == mpack_node_i8(node));
    test_simple_tree_read("\xd0\x80", INT8_MIN == mpack_node_i16(node));
    test_simple_tree_read("\xd0\x80", INT8_MIN == mpack_node_i32(node));
    test_simple_tree_read("\xd0\x80", INT8_MIN == mpack_node_i64(node));

    test_simple_tree_read("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i16(node));
    test_simple_tree_read("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i32(node));
    test_simple_tree_read("\xd1\xff\x7f", INT8_MIN - 1 == mpack_node_i64(node));

    test_simple_tree_read("\xd1\x80\x00", INT16_MIN == mpack_node_i16(node));
    test_simple_tree_read("\xd1\x80\x00", INT16_MIN == mpack_node_i32(node));
    test_simple_tree_read("\xd1\x80\x00", INT16_MIN == mpack_node_i64(node));

    test_simple_tree_read("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_node_i32(node));
    test_simple_tree_read("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_node_i64(node));

    test_simple_tree_read("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_node_i32(node));
    test_simple_tree_read("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_node_i64(node));

    test_simple_tree_read("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", (int64_t)INT32_MIN - 1 == mpack_node_i64(node));

    test_simple_tree_read("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", INT64_MIN == mpack_node_i64(node));

}

static void test_node_read_ints_dynamic_int() {
    mpack_node_t nodes[128];

    // we don't bother to test with different signed/unsigned value
    // functions; they are tested for equality in test-value.c

    // positive fixnums
    test_simple_tree_read("\x00", mpack_tag_equal(mpack_tag_uint(0), mpack_node_tag(node)));
    test_simple_tree_read("\x01", mpack_tag_equal(mpack_tag_uint(1), mpack_node_tag(node)));
    test_simple_tree_read("\x02", mpack_tag_equal(mpack_tag_uint(2), mpack_node_tag(node)));
    test_simple_tree_read("\x0f", mpack_tag_equal(mpack_tag_uint(0x0f), mpack_node_tag(node)));
    test_simple_tree_read("\x10", mpack_tag_equal(mpack_tag_uint(0x10), mpack_node_tag(node)));
    test_simple_tree_read("\x7f", mpack_tag_equal(mpack_tag_uint(0x7f), mpack_node_tag(node)));

    // negative fixnums
    test_simple_tree_read("\xff", mpack_tag_equal(mpack_tag_int(-1), mpack_node_tag(node)));
    test_simple_tree_read("\xfe", mpack_tag_equal(mpack_tag_int(-2), mpack_node_tag(node)));
    test_simple_tree_read("\xf0", mpack_tag_equal(mpack_tag_int(-16), mpack_node_tag(node)));
    test_simple_tree_read("\xe0", mpack_tag_equal(mpack_tag_int(-32), mpack_node_tag(node)));

    // uints
    test_simple_tree_read("\xcc\x80", mpack_tag_equal(mpack_tag_uint(0x80), mpack_node_tag(node)));
    test_simple_tree_read("\xcc\xff", mpack_tag_equal(mpack_tag_uint(0xff), mpack_node_tag(node)));
    test_simple_tree_read("\xcd\x01\x00", mpack_tag_equal(mpack_tag_uint(0x100), mpack_node_tag(node)));
    test_simple_tree_read("\xcd\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffff), mpack_node_tag(node)));
    test_simple_tree_read("\xce\x00\x01\x00\x00", mpack_tag_equal(mpack_tag_uint(0x10000), mpack_node_tag(node)));
    test_simple_tree_read("\xce\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffffffff), mpack_node_tag(node)));
    test_simple_tree_read("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_uint(UINT64_C(0x100000000)), mpack_node_tag(node)));
    test_simple_tree_read("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(UINT64_C(0xffffffffffffffff)), mpack_node_tag(node)));

    // ints
    test_simple_tree_read("\xd0\xdf", mpack_tag_equal(mpack_tag_int(-33), mpack_node_tag(node)));
    test_simple_tree_read("\xd0\x80", mpack_tag_equal(mpack_tag_int(INT8_MIN), mpack_node_tag(node)));
    test_simple_tree_read("\xd1\xff\x7f", mpack_tag_equal(mpack_tag_int(INT8_MIN - 1), mpack_node_tag(node)));
    test_simple_tree_read("\xd1\x80\x00", mpack_tag_equal(mpack_tag_int(INT16_MIN), mpack_node_tag(node)));
    test_simple_tree_read("\xd2\xff\xff\x7f\xff", mpack_tag_equal(mpack_tag_int(INT16_MIN - 1), mpack_node_tag(node)));

    test_simple_tree_read("\xd2\x80\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT32_MIN), mpack_node_tag(node)));
    test_simple_tree_read("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_tag_equal(mpack_tag_int((int64_t)INT32_MIN - 1), mpack_node_tag(node)));

    test_simple_tree_read("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT64_MIN), mpack_node_tag(node)));

}

static void test_node_read_int_bounds() {
    mpack_node_t nodes[128];

    test_simple_tree_read_error("\xd1\xff\x7f", 0 == mpack_node_i8(node), mpack_error_type); 
    test_simple_tree_read_error("\xd1\x80\x00", 0 == mpack_node_i8(node), mpack_error_type);

    test_simple_tree_read_error("\xd2\xff\xff\x7f\xff", 0 == mpack_node_i8(node), mpack_error_type);
    test_simple_tree_read_error("\xd2\xff\xff\x7f\xff", 0 == mpack_node_i16(node), mpack_error_type);

    test_simple_tree_read_error("\xd2\x80\x00\x00\x00", 0 == mpack_node_i8(node), mpack_error_type);
    test_simple_tree_read_error("\xd2\x80\x00\x00\x00", 0 == mpack_node_i16(node), mpack_error_type);

    test_simple_tree_read_error("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i8(node), mpack_error_type);
    test_simple_tree_read_error("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i16(node), mpack_error_type);
    test_simple_tree_read_error("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_node_i32(node), mpack_error_type);

    test_simple_tree_read_error("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i8(node), mpack_error_type);
    test_simple_tree_read_error("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i16(node), mpack_error_type);
    test_simple_tree_read_error("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_node_i32(node), mpack_error_type);

}

static void test_node_read_uint_bounds() {
    mpack_node_t nodes[128];

    test_simple_tree_read_error("\xcd\x01\x00", 0 == mpack_node_u8(node), mpack_error_type);
    test_simple_tree_read_error("\xcd\xff\xff", 0 == mpack_node_u8(node), mpack_error_type);

    test_simple_tree_read_error("\xce\x00\x01\x00\x00", 0 == mpack_node_u8(node), mpack_error_type);
    test_simple_tree_read_error("\xce\x00\x01\x00\x00", 0 == mpack_node_u16(node), mpack_error_type);

    test_simple_tree_read_error("\xce\xff\xff\xff\xff", 0 == mpack_node_u8(node), mpack_error_type);
    test_simple_tree_read_error("\xce\xff\xff\xff\xff", 0 == mpack_node_u16(node), mpack_error_type);

    test_simple_tree_read_error("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u8(node), mpack_error_type);
    test_simple_tree_read_error("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u16(node), mpack_error_type);
    test_simple_tree_read_error("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_node_u32(node), mpack_error_type);

}

static void test_node_read_misc() {
    mpack_node_t nodes[128];

    test_simple_tree_read("\xc0", (mpack_node_nil(node), true));

    test_simple_tree_read("\xc2", false == mpack_node_bool(node));
    test_simple_tree_read("\xc2", (mpack_node_false(node), true));
    test_simple_tree_read("\xc3", true == mpack_node_bool(node));
    test_simple_tree_read("\xc3", (mpack_node_true(node), true));

    test_simple_tree_read_error("\xc2", (mpack_node_true(node), true), mpack_error_type);
    test_simple_tree_read_error("\xc3", (mpack_node_false(node), true), mpack_error_type);
}

static void test_node_read_floats() {
    mpack_node_t nodes[128];

    // these are some very simple floats that don't really test IEEE 742 conformance;
    // this section could use some improvement

    test_simple_tree_read("\x00", 0.0f == mpack_node_float(node));
    test_simple_tree_read("\xd0\x00", 0.0f == mpack_node_float(node));
    test_simple_tree_read("\xca\x00\x00\x00\x00", 0.0f == mpack_node_float(node));
    test_simple_tree_read("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_node_float(node));

    test_simple_tree_read("\x00", 0.0 == mpack_node_double(node));
    test_simple_tree_read("\xd0\x00", 0.0 == mpack_node_double(node));
    test_simple_tree_read("\xca\x00\x00\x00\x00", 0.0 == mpack_node_double(node));
    test_simple_tree_read("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0 == mpack_node_double(node));

    test_simple_tree_read("\xca\xff\xff\xff\xff", isnanf(mpack_node_float(node)));
    test_simple_tree_read("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnanf(mpack_node_float(node)));
    test_simple_tree_read("\xca\xff\xff\xff\xff", isnan(mpack_node_double(node)));
    test_simple_tree_read("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_node_double(node)));

    test_simple_tree_read("\xca\x00\x00\x00\x00", 0.0f == mpack_node_float_strict(node));
    test_simple_tree_read("\xca\x00\x00\x00\x00", 0.0 == mpack_node_double_strict(node));
    test_simple_tree_read("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0 == mpack_node_double_strict(node));
    test_simple_tree_read("\xca\xff\xff\xff\xff", isnanf(mpack_node_float_strict(node)));
    test_simple_tree_read("\xca\xff\xff\xff\xff", isnan(mpack_node_double_strict(node)));
    test_simple_tree_read("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_node_double_strict(node)));

    test_simple_tree_read_error("\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    test_simple_tree_read_error("\xd0\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    test_simple_tree_read_error("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_node_float_strict(node), mpack_error_type);

    test_simple_tree_read_error("\x00", 0.0 == mpack_node_double_strict(node), mpack_error_type);
    test_simple_tree_read_error("\xd0\x00", 0.0 == mpack_node_double_strict(node), mpack_error_type);
}

static void test_node_read_bad_type() {
    mpack_node_t nodes[128];

    // test that non-compound node functions correctly handle badly typed data
    test_simple_tree_read_error("\xc2", (mpack_node_nil(node), true), mpack_error_type);
    test_simple_tree_read_error("\xc0", false == mpack_node_bool(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_u8(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_u16(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_u32(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_u64(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_i8(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_i16(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_i32(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0 == mpack_node_i64(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0.0f == mpack_node_float(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0.0 == mpack_node_double(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0.0f == mpack_node_float_strict(node), mpack_error_type);
    test_simple_tree_read_error("\xc0", 0.0 == mpack_node_double_strict(node), mpack_error_type);
}

static void test_node_read_pre_error() {
    mpack_node_t nodes[128];

    // test that all node functions correctly handle pre-existing errors

    test_simple_tree_read_error("", (mpack_node_nil(node), true), mpack_error_io);
    test_simple_tree_read_error("", false == mpack_node_bool(node), mpack_error_io);

    test_simple_tree_read_error("", 0 == mpack_node_u8(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_u16(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_u32(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_u64(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_i8(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_i16(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_i32(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_i64(node), mpack_error_io);

    test_simple_tree_read_error("", 0.0f == mpack_node_float(node), mpack_error_io);
    test_simple_tree_read_error("", 0.0 == mpack_node_double(node), mpack_error_io);
    test_simple_tree_read_error("", 0.0f == mpack_node_float_strict(node), mpack_error_io);
    test_simple_tree_read_error("", 0.0 == mpack_node_double_strict(node), mpack_error_io);

    test_simple_tree_read_error("", 0 == mpack_node_array_length(node), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_array_at(node, 0), mpack_error_io);

    test_simple_tree_read_error("", 0 == mpack_node_map_count(node), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_map_key_at(node, 0), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_map_value_at(node, 0), mpack_error_io);

    test_simple_tree_read_error("", mpack_node_map_uint(node, 1), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_map_int(node, -1), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_map_str(node, "test", 4), mpack_error_io);
    test_simple_tree_read_error("", mpack_node_map_cstr(node, "test"), mpack_error_io);
    test_simple_tree_read_error("", false == mpack_node_map_contains_str(node, "test", 4), mpack_error_io);
    test_simple_tree_read_error("", false == mpack_node_map_contains_cstr(node, "test"), mpack_error_io);

    test_simple_tree_read_error("", 0 == mpack_node_exttype(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_data_len(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_strlen(node), mpack_error_io);
    test_simple_tree_read_error("", NULL == mpack_node_data(node), mpack_error_io);
    test_simple_tree_read_error("", 0 == mpack_node_copy_data(node, NULL, 0), mpack_error_io);
    test_simple_tree_read_error("", (mpack_node_copy_cstr(node, NULL, 0), true), mpack_error_io);
    #ifdef MPACK_MALLOC
    test_simple_tree_read_error("", NULL == mpack_node_data_alloc(node, 0), mpack_error_io);
    test_simple_tree_read_error("", NULL == mpack_node_cstr_alloc(node, 0), mpack_error_io);
    #endif
}

static void test_node_read_array() {
    static const char test[] = "\x93\x90\x91\xc3\x92\xc3\xc3";
    mpack_tree_t tree;
    test_tree_init(&tree, test, sizeof(test) - 1);
    mpack_node_t* root = mpack_tree_root(&tree);

    test_assert(mpack_type_array == mpack_node_type(root));
    test_assert(3 == mpack_node_array_length(root));

    test_assert(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 0)));
    test_assert(0 == mpack_node_array_length(mpack_node_array_at(root, 0)));

    test_assert(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 1)));
    test_assert(1 == mpack_node_array_length(mpack_node_array_at(root, 1)));
    test_assert(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 1), 0)));
    test_assert(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 1), 0)));

    test_assert(mpack_type_array == mpack_node_type(mpack_node_array_at(root, 2)));
    test_assert(2 == mpack_node_array_length(mpack_node_array_at(root, 2)));
    test_assert(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 2), 0)));
    test_assert(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 2), 0)));
    test_assert(mpack_type_bool == mpack_node_type(mpack_node_array_at(mpack_node_array_at(root, 2), 1)));
    test_assert(true == mpack_node_bool(mpack_node_array_at(mpack_node_array_at(root, 2), 1)));

    test_assert(mpack_ok == mpack_tree_error(&tree));

    // test out of bounds
    test_assert(mpack_type_nil == mpack_node_type(mpack_node_array_at(root, 4)));
    test_tree_destroy_error(&tree, mpack_error_data);
}

static void test_node_read_map() {
    // test map using maps as keys and values
    static const char test[] = "\x82\x80\x81\x01\x02\x81\x03\x04\xc3";
    mpack_tree_t tree;
    test_tree_init(&tree, test, sizeof(test) - 1);
    mpack_node_t* root = mpack_tree_root(&tree);

    test_assert(mpack_type_map == mpack_node_type(root));
    test_assert(2 == mpack_node_map_count(root));

    test_assert(mpack_type_map == mpack_node_type(mpack_node_map_key_at(root, 0)));
    test_assert(0 == mpack_node_map_count(mpack_node_map_key_at(root, 0)));

    test_assert(mpack_type_map == mpack_node_type(mpack_node_map_value_at(root, 0)));
    test_assert(1 == mpack_node_map_count(mpack_node_map_value_at(root, 0)));
    test_assert(1 == mpack_node_i32(mpack_node_map_key_at(mpack_node_map_value_at(root, 0), 0)));
    test_assert(2 == mpack_node_i32(mpack_node_map_value_at(mpack_node_map_value_at(root, 0), 0)));

    test_assert(mpack_type_map == mpack_node_type(mpack_node_map_key_at(root, 1)));
    test_assert(1 == mpack_node_map_count(mpack_node_map_key_at(root, 1)));
    test_assert(3 == mpack_node_i32(mpack_node_map_key_at(mpack_node_map_key_at(root, 1), 0)));
    test_assert(4 == mpack_node_i32(mpack_node_map_value_at(mpack_node_map_key_at(root, 1), 0)));

    test_assert(mpack_type_bool == mpack_node_type(mpack_node_map_value_at(root, 1)));
    test_assert(true == mpack_node_bool(mpack_node_map_value_at(root, 1)));

    test_assert(mpack_ok == mpack_tree_error(&tree));

    // test out of bounds
    test_assert(mpack_type_nil == mpack_node_type(mpack_node_map_key_at(root, 2)));
    test_tree_destroy_error(&tree, mpack_error_data);
}

static void test_node_read_map_search() {
    static const char test[] = "\x85\x00\x01\xd0\x7f\x02\xfe\x03\xa5""alice\x04\xa3""bob\x05";
    mpack_tree_t tree;
    test_tree_init(&tree, test, sizeof(test) - 1);
    mpack_node_t* root = mpack_tree_root(&tree);

    test_assert(1 == mpack_node_i32(mpack_node_map_uint(root, 0)));
    test_assert(1 == mpack_node_i32(mpack_node_map_int(root, 0)));
    test_assert(2 == mpack_node_i32(mpack_node_map_uint(root, 127))); // underlying tag type is int
    test_assert(3 == mpack_node_i32(mpack_node_map_int(root, -2)));
    test_assert(4 == mpack_node_i32(mpack_node_map_str(root, "alice", 5)));
    test_assert(5 == mpack_node_i32(mpack_node_map_cstr(root, "bob")));

    test_assert(true == mpack_node_map_contains_str(root, "alice", 5));
    test_assert(true == mpack_node_map_contains_cstr(root, "bob"));
    test_assert(false == mpack_node_map_contains_str(root, "eve", 3));
    test_assert(false == mpack_node_map_contains_cstr(root, "eve"));

    test_tree_destroy_noerror(&tree);
}

static void test_node_read_compound_errors(void) {
    mpack_node_t nodes[128];

    test_simple_tree_read_error("\x00", 0 == mpack_node_array_length(node), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_array_at(node, 0), mpack_error_type);
    test_simple_tree_read_error("\x00", 0 == mpack_node_map_count(node), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_map_key_at(node, 0), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_map_value_at(node, 0), mpack_error_type);

    test_simple_tree_read_error("\x80", mpack_node_map_int(node, -1), mpack_error_data);
    test_simple_tree_read_error("\x80", mpack_node_map_uint(node, 1), mpack_error_data);
    test_simple_tree_read_error("\x80", mpack_node_map_str(node, "test", 4), mpack_error_data);
    test_simple_tree_read_error("\x80", mpack_node_map_cstr(node, "test"), mpack_error_data);

    test_simple_tree_read_error("\x00", mpack_node_map_int(node, -1), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_map_uint(node, 1), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_map_str(node, "test", 4), mpack_error_type);
    test_simple_tree_read_error("\x00", mpack_node_map_cstr(node, "test"), mpack_error_type);
    test_simple_tree_read_error("\x00", false == mpack_node_map_contains_str(node, "test", 4), mpack_error_type);
    test_simple_tree_read_error("\x00", false == mpack_node_map_contains_cstr(node, "test"), mpack_error_type);

    test_simple_tree_read_error("\x00", 0 == mpack_node_exttype(node), mpack_error_type);
    test_simple_tree_read_error("\x00", 0 == mpack_node_data_len(node), mpack_error_type);
    test_simple_tree_read_error("\x00", 0 == mpack_node_strlen(node), mpack_error_type);
    test_simple_tree_read_error("\x00", NULL == mpack_node_data(node), mpack_error_type);
    test_simple_tree_read_error("\x00", 0 == mpack_node_copy_data(node, NULL, 0), mpack_error_type);

    char data[1] = {'a'};
    test_simple_tree_read_error("\x00", (mpack_node_copy_data(node, data, 1), true), mpack_error_type);
    test_assert(data[0] == 'a');
    test_simple_tree_read_error("\x00", (mpack_node_copy_cstr(node, data, 1), true), mpack_error_type);
    test_assert(data[0] == 0);

    #ifdef MPACK_MALLOC
    test_simple_tree_read_error("\x00", NULL == mpack_node_data_alloc(node, 10), mpack_error_type);
    test_simple_tree_read_error("\x00", NULL == mpack_node_cstr_alloc(node, 10), mpack_error_type);
    #endif

    data[0] = 'a';
    test_simple_tree_read_error("\xa3""bob", (mpack_node_copy_data(node, data, 2), true), mpack_error_too_big);
    test_assert(data[0] == 'a');
    test_simple_tree_read_error("\xa3""bob", (mpack_node_copy_cstr(node, data, 2), true), mpack_error_too_big);
    test_assert(data[0] == 0);

    #ifdef MPACK_MALLOC
    test_simple_tree_read_error("\xa3""bob", NULL == mpack_node_cstr_alloc(node, 2), mpack_error_too_big);
    test_simple_tree_read_error("\xa3""bob", NULL == mpack_node_data_alloc(node, 2), mpack_error_too_big);
    #endif
}

static void test_node_read_data(void) {
    static const char test[] = "\x93\xa5""alice\xc4\x03""bob\xd6\x07""carl";
    mpack_tree_t tree;
    test_tree_init(&tree, test, sizeof(test) - 1);
    mpack_node_t* root = mpack_tree_root(&tree);

    mpack_node_t* alice = mpack_node_array_at(root, 0);
    test_assert(5 == mpack_node_data_len(alice));
    test_assert(5 == mpack_node_strlen(alice));
    test_assert(NULL != mpack_node_data(alice));
    test_assert(0 == memcmp("alice", mpack_node_data(alice), 5));

    char alice_data[6] = {'s','s','s','s','s','s'};
    mpack_node_copy_data(alice, alice_data, sizeof(alice_data));
    test_assert(0 == memcmp("alices", alice_data, 6));
    mpack_node_copy_cstr(alice, alice_data, sizeof(alice_data));
    test_assert(0 == strcmp("alice", alice_data));

    #ifdef MPACK_MALLOC
    char* alice_alloc = mpack_node_cstr_alloc(alice, 100);
    test_assert(0 == strcmp("alice", alice_alloc));
    free(alice_alloc);
    #endif

    mpack_node_t* bob = mpack_node_array_at(root, 1);
    test_assert(3 == mpack_node_data_len(bob));
    test_assert(0 == memcmp("bob", mpack_node_data(bob), 3));

    #ifdef MPACK_MALLOC
    char* bob_alloc = mpack_node_data_alloc(bob, 100);
    test_assert(0 == memcmp("bob", bob_alloc, 3));
    free(bob_alloc);
    #endif

    mpack_node_t* carl = mpack_node_array_at(root, 2);
    test_assert(7 == mpack_node_exttype(carl));
    test_assert(4 == mpack_node_data_len(carl));
    test_assert(0 == memcmp("carl", mpack_node_data(carl), 4));

    test_tree_destroy_noerror(&tree);
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
    test_node_read_pre_error();

    // compound types
    test_node_read_array();
    test_node_read_map();
    test_node_read_map_search();
    test_node_read_compound_errors();
    test_node_read_data();
}

#endif

