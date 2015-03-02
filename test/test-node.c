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

// tests the example on the messagepack homepage
static void test_example_node() {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";
    mpack_tree_t tree;
    mpack_tree_init(&tree, test, sizeof(test) - 1);
    //mpack_node_print(mpack_tree_root(&tree));

    if (MPACK_TREE_SETJMP(&tree)) {
        test_assert(0, "jumped! error: %s", mpack_error_to_string(mpack_tree_error(&tree)));
        return;
    }

    mpack_node_t* map = mpack_tree_root(&tree);
    test_assert(true == mpack_node_bool(mpack_node_map_cstr(map, "compact")));
    test_assert(0 == mpack_node_u8(mpack_node_map_cstr(map, "schema")));

    test_tree_destroy_noerror(&tree);
}

static void test_node_read_uint_fixnum() {

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

    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i8(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i16(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i32(node));
    test_simple_tree_read("\xd0\xdf", -33 == mpack_node_i64(node));

    test_simple_tree_read("\xd0\x80", -128 == mpack_node_i8(node));
    test_simple_tree_read("\xd0\x80", -128 == mpack_node_i16(node));
    test_simple_tree_read("\xd0\x80", -128 == mpack_node_i32(node));
    test_simple_tree_read("\xd0\x80", -128 == mpack_node_i64(node));

    test_simple_tree_read("\xd1\xff\x7f", -129 == mpack_node_i16(node));
    test_simple_tree_read("\xd1\xff\x7f", -129 == mpack_node_i32(node));
    test_simple_tree_read("\xd1\xff\x7f", -129 == mpack_node_i64(node));

    test_simple_tree_read("\xd1\x80\x00", -32768 == mpack_node_i16(node));
    test_simple_tree_read("\xd1\x80\x00", -32768 == mpack_node_i32(node));
    test_simple_tree_read("\xd1\x80\x00", -32768 == mpack_node_i64(node));

    test_simple_tree_read("\xd2\xff\xff\x7f\xff", -32769 == mpack_node_i32(node));
    test_simple_tree_read("\xd2\xff\xff\x7f\xff", -32769 == mpack_node_i64(node));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    test_simple_tree_read("\xd2\x80\x00\x00\x00", INT64_C(-2147483648) == mpack_node_i32(node));
    test_simple_tree_read("\xd2\x80\x00\x00\x00", INT64_C(-2147483648) == mpack_node_i64(node));

    test_simple_tree_read("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", INT64_C(-2147483649) == mpack_node_i64(node));

    test_simple_tree_read("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", INT64_MIN == mpack_node_i64(node));

}

static void test_node_read_ints_dynamic_int() {

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
    test_simple_tree_read("\xd0\x80", mpack_tag_equal(mpack_tag_int(-128), mpack_node_tag(node)));
    test_simple_tree_read("\xd1\xff\x7f", mpack_tag_equal(mpack_tag_int(-129), mpack_node_tag(node)));
    test_simple_tree_read("\xd1\x80\x00", mpack_tag_equal(mpack_tag_int(-32768), mpack_node_tag(node)));
    test_simple_tree_read("\xd2\xff\xff\x7f\xff", mpack_tag_equal(mpack_tag_int(-32769), mpack_node_tag(node)));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    test_simple_tree_read("\xd2\x80\x00\x00\x00", mpack_tag_equal(mpack_tag_int(UINT64_C(-2147483648)), mpack_node_tag(node)));
    test_simple_tree_read("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_tag_equal(mpack_tag_int(UINT64_C(-2147483649)), mpack_node_tag(node)));

    test_simple_tree_read("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT64_MIN), mpack_node_tag(node)));

}

static void test_node_read_int_bounds() {

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
    test_simple_tree_read("\xc0", (mpack_node_nil(node), true));
    test_simple_tree_read("\xc2", false == mpack_node_bool(node));
    test_simple_tree_read("\xc3", true == mpack_node_bool(node));
}

static void test_node_read_floats() {
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
    // test that all node functions correctly handle badly typed data
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
}

