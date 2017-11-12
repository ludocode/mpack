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

#if MPACK_EXPECT

static const char test_example[] = "\x82\xA7""compact\xC3\xA6""schema\x00";
#define TEST_EXAMPLE_SIZE (sizeof(test_example) - 1)

static void test_expect_example_read() {
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, test_example, TEST_EXAMPLE_SIZE);

    // tests the example on the messagepack homepage using fixed
    // ordering for keys (mpack_expect_key_cstr_basic() tests re-ordering)
    TEST_TRUE(2 == mpack_expect_map(&reader));
    mpack_expect_cstr_match(&reader, "compact");
    TEST_TRUE(true == mpack_expect_bool(&reader));
    mpack_expect_cstr_match(&reader, "schema");
    TEST_TRUE(0 == mpack_expect_u8(&reader));
    mpack_done_map(&reader);

    TEST_READER_DESTROY_NOERROR(&reader);
}

/*
   [
   1,-32,128,-129,65536,-65537,2147483649,-2147483648,9223372036854775807,-9223372036854775808,
   ]
   */

static void test_expect_uint_fixnum() {

    // positive fixnums with u8
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_u8(&reader));

    // positive fixnums with u16
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_u16(&reader));

    // positive fixnums with u32
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_u32(&reader));

    // positive fixnums with u64
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_u64(&reader));

}

static void test_expect_uint_signed_fixnum() {

    // positive fixnums with i8
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_i8(&reader));

    // positive fixnums with i16
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_i16(&reader));

    // positive fixnums with i32
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_i32(&reader));

    // positive fixnums with i64
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\x0f", 0x0f == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\x10", 0x10 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\x7f", 0x7f == mpack_expect_i64(&reader));

}

static void test_expect_negative_fixnum() {

    // negative fixnums with i8
    TEST_SIMPLE_READ("\xff", -1 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\xfe", -2 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\xf0", -16 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\xe0", -32 == mpack_expect_i8(&reader));

    // negative fixnums with i16
    TEST_SIMPLE_READ("\xff", -1 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xfe", -2 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xf0", -16 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xe0", -32 == mpack_expect_i16(&reader));

    // negative fixnums with i32
    TEST_SIMPLE_READ("\xff", -1 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xfe", -2 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xf0", -16 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xe0", -32 == mpack_expect_i32(&reader));

    // negative fixnums with i64
    TEST_SIMPLE_READ("\xff", -1 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xfe", -2 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xf0", -16 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xe0", -32 == mpack_expect_i64(&reader));

}

static void test_expect_uint() {

    // positive signed into u8
    TEST_SIMPLE_READ("\xd0\x7f", 0x7f == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\xd0\x7f", 0x7f == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xd0\x7f", 0x7f == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xd0\x7f", 0x7f == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xd0\x7f", 0x7f == mpack_expect_uint(&reader));
    TEST_SIMPLE_READ("\xd1\x7f\xff", 0x7fff == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xd1\x7f\xff", 0x7fff == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xd1\x7f\xff", 0x7fff == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xd1\x7f\xff", 0x7fff == mpack_expect_uint(&reader));
    TEST_SIMPLE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xd2\x7f\xff\xff\xff", 0x7fffffff == mpack_expect_uint(&reader));
    TEST_SIMPLE_READ("\xd3\x7f\xff\xff\xff\xff\xff\xff\xff", 0x7fffffffffffffff == mpack_expect_u64(&reader));

    // positive unsigned into u8
    
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_u8(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_u16(&reader));
    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_expect_u32(&reader));
    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_expect_uint(&reader));

    TEST_SIMPLE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_expect_u64(&reader));
    TEST_SIMPLE_READ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 0xffffffffffffffff == mpack_expect_u64(&reader));

}

static void test_expect_uint_signed() {

    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xcc\x80", 0x80 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xcc\xff", 0xff == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xcd\x01\x00", 0x100 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xcd\xff\xff", 0xffff == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", 0x10000 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", 0xffffffff == mpack_expect_i64(&reader));

    TEST_SIMPLE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0x100000000 == mpack_expect_i64(&reader));

}

static void test_expect_int() {

    TEST_SIMPLE_READ("\xd0\xdf", -33 == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\xd0\xdf", -33 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xd0\xdf", -33 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd0\xdf", -33 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd0\xdf", -33 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd0\x80", INT8_MIN == mpack_expect_i8(&reader));
    TEST_SIMPLE_READ("\xd0\x80", INT8_MIN == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xd0\x80", INT8_MIN == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd0\x80", INT8_MIN == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd0\x80", INT8_MIN == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd1\xff\x7f", INT8_MIN - 1 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd1\x80\x00", INT16_MIN == mpack_expect_i16(&reader));
    TEST_SIMPLE_READ("\xd1\x80\x00", INT16_MIN == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd1\x80\x00", INT16_MIN == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd1\x80\x00", INT16_MIN == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd2\xff\xff\x7f\xff", INT16_MIN - 1 == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_expect_i32(&reader));
    TEST_SIMPLE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_expect_i64(&reader));
    TEST_SIMPLE_READ("\xd2\x80\x00\x00\x00", INT32_MIN == mpack_expect_int(&reader));

    TEST_SIMPLE_READ("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", (int64_t)INT32_MIN - 1 == mpack_expect_i64(&reader));

    TEST_SIMPLE_READ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", INT64_MIN == mpack_expect_i64(&reader));

}

static void test_expect_ints_dynamic_int() {

    // we don't bother to test with different signed/unsigned value
    // functions; they are tested for equality in test-value.c

    // positive fixnums
    TEST_SIMPLE_READ("\x00", mpack_tag_equal(mpack_tag_uint(0), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\x01", mpack_tag_equal(mpack_tag_uint(1), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\x02", mpack_tag_equal(mpack_tag_uint(2), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\x0f", mpack_tag_equal(mpack_tag_uint(0x0f), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\x10", mpack_tag_equal(mpack_tag_uint(0x10), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\x7f", mpack_tag_equal(mpack_tag_uint(0x7f), mpack_read_tag(&reader)));

    // negative fixnums
    TEST_SIMPLE_READ("\xff", mpack_tag_equal(mpack_tag_int(-1), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xfe", mpack_tag_equal(mpack_tag_int(-2), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xf0", mpack_tag_equal(mpack_tag_int(-16), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xe0", mpack_tag_equal(mpack_tag_int(-32), mpack_read_tag(&reader)));

    // uints
    TEST_SIMPLE_READ("\xcc\x80", mpack_tag_equal(mpack_tag_uint(0x80), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xcc\xff", mpack_tag_equal(mpack_tag_uint(0xff), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xcd\x01\x00", mpack_tag_equal(mpack_tag_uint(0x100), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xcd\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffff), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", mpack_tag_equal(mpack_tag_uint(0x10000), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(0xffffffff), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_uint(UINT64_C(0x100000000)), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_tag_equal(mpack_tag_uint(UINT64_C(0xffffffffffffffff)), mpack_read_tag(&reader)));

    // ints
    TEST_SIMPLE_READ("\xd0\xdf", mpack_tag_equal(mpack_tag_int(-33), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xd0\x80", mpack_tag_equal(mpack_tag_int(INT8_MIN), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xd1\xff\x7f", mpack_tag_equal(mpack_tag_int(INT8_MIN - 1), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xd1\x80\x00", mpack_tag_equal(mpack_tag_int(INT16_MIN), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xd2\xff\xff\x7f\xff", mpack_tag_equal(mpack_tag_int(INT16_MIN - 1), mpack_read_tag(&reader)));

    TEST_SIMPLE_READ("\xd2\x80\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT32_MIN), mpack_read_tag(&reader)));
    TEST_SIMPLE_READ("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_tag_equal(mpack_tag_int((int64_t)INT32_MIN - 1), mpack_read_tag(&reader)));

    TEST_SIMPLE_READ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_tag_equal(mpack_tag_int(INT64_MIN), mpack_read_tag(&reader)));

}

static void test_expect_int_bounds() {

    TEST_SIMPLE_READ_ERROR("\xd1\xff\x7f", 0 == mpack_expect_i8(&reader), mpack_error_type); 
    TEST_SIMPLE_READ_ERROR("\xd1\x80\x00", 0 == mpack_expect_i8(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xd2\xff\xff\x7f\xff", 0 == mpack_expect_i8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd2\xff\xff\x7f\xff", 0 == mpack_expect_i16(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xd2\x80\x00\x00\x00", 0 == mpack_expect_i8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd2\x80\x00\x00\x00", 0 == mpack_expect_i16(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_expect_i8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_expect_i16(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 0 == mpack_expect_i32(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_expect_i8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_expect_i16(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 0 == mpack_expect_i32(&reader), mpack_error_type);

}

static void test_expect_uint_bounds() {

    TEST_SIMPLE_READ_ERROR("\xcd\x01\x00", 0 == mpack_expect_u8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xcd\xff\xff", 0 == mpack_expect_u8(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xce\x00\x01\x00\x00", 0 == mpack_expect_u8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xce\x00\x01\x00\x00", 0 == mpack_expect_u16(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xce\xff\xff\xff\xff", 0 == mpack_expect_u8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xce\xff\xff\xff\xff", 0 == mpack_expect_u16(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_expect_u8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_expect_u16(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 0 == mpack_expect_u32(&reader), mpack_error_type);

}

#define TEST_EXPECT_UINT_RANGE(name) \
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_##name##_max(&reader, 0));                                  \
    TEST_SIMPLE_READ_ERROR("\x01", 0 == mpack_expect_##name##_max(&reader, 0), mpack_error_type);          \
                                                                                                           \
    TEST_SIMPLE_READ_ERROR("\x00", 1 == mpack_expect_##name##_range(&reader, 1, 3), mpack_error_type);     \
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_##name##_range(&reader, 1, 3));                             \
    TEST_SIMPLE_READ("\x02", 2 == mpack_expect_##name##_range(&reader, 1, 3));                             \
    TEST_SIMPLE_READ("\x03", 3 == mpack_expect_##name##_range(&reader, 1, 3));                             \
    TEST_SIMPLE_READ_ERROR("\x04", 1 == mpack_expect_##name##_range(&reader, 1, 3), mpack_error_type);     \
                                                                                                           \
    TEST_SIMPLE_READ_ASSERT("\x00", mpack_expect_##name##_range(reader, 1, 0));

#define TEST_EXPECT_INT_RANGE(name) \
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_##name##_max(&reader, 0));                                  \
    TEST_SIMPLE_READ_ERROR("\x01", 0 == mpack_expect_##name##_max(&reader, 0), mpack_error_type);          \
    TEST_SIMPLE_READ_ERROR("\xff", 0 == mpack_expect_##name##_max(&reader, 0), mpack_error_type);          \
                                                                                                           \
    TEST_SIMPLE_READ_ERROR("\xfe", -1 == mpack_expect_##name##_range(&reader, -1, 1), mpack_error_type);   \
    TEST_SIMPLE_READ("\xff", -1 == mpack_expect_##name##_range(&reader, -1, 1));                           \
    TEST_SIMPLE_READ("\x00", 0 == mpack_expect_##name##_range(&reader, -1, 1));                            \
    TEST_SIMPLE_READ("\x01", 1 == mpack_expect_##name##_range(&reader, -1, 1));                            \
    TEST_SIMPLE_READ_ERROR("\x02", -1 == mpack_expect_##name##_range(&reader, -1, 1), mpack_error_type);   \
                                                                                                           \
    TEST_SIMPLE_READ_ASSERT("\x00", mpack_expect_##name##_range(reader, 1, -1));

static void test_expect_int_range() {
    // these currently don't test anything involving the limits of
    // each data type; there doesn't seem to be much point in doing
    // so, since they all wrap the normal expect functions.
    TEST_EXPECT_UINT_RANGE(u8);
    TEST_EXPECT_UINT_RANGE(u16);
    TEST_EXPECT_UINT_RANGE(u32);
    TEST_EXPECT_UINT_RANGE(u64);
    TEST_EXPECT_UINT_RANGE(uint);
    TEST_EXPECT_INT_RANGE(i8);
    TEST_EXPECT_INT_RANGE(i16);
    TEST_EXPECT_INT_RANGE(i32);
    TEST_EXPECT_INT_RANGE(i64);
    TEST_EXPECT_INT_RANGE(int);
}

static void test_expect_int_match() {
    TEST_SIMPLE_READ("\x00", (mpack_expect_uint_match(&reader, 0), true));
    TEST_SIMPLE_READ("\x01", (mpack_expect_uint_match(&reader, 1), true));
    TEST_SIMPLE_READ("\xcc\x80", (mpack_expect_uint_match(&reader, 0x80), true));
    TEST_SIMPLE_READ("\xcc\xff", (mpack_expect_uint_match(&reader, 0xff), true));
    TEST_SIMPLE_READ("\xcd\x01\x00", (mpack_expect_uint_match(&reader, 0x100), true));
    TEST_SIMPLE_READ("\xcd\xff\xff", (mpack_expect_uint_match(&reader, 0xffff), true));
    TEST_SIMPLE_READ("\xce\x00\x01\x00\x00", (mpack_expect_uint_match(&reader, 0x10000), true));
    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", (mpack_expect_uint_match(&reader, 0xffffffff), true));
    TEST_SIMPLE_READ("\xce\xff\xff\xff\xff", (mpack_expect_uint_match(&reader, 0xffffffff), true));
    TEST_SIMPLE_READ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", (mpack_expect_uint_match(&reader, 0x100000000), true));
    TEST_SIMPLE_READ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", (mpack_expect_uint_match(&reader, 0xffffffffffffffff), true));
    TEST_SIMPLE_READ_ERROR("\xff", (mpack_expect_uint_match(&reader, 0), true), mpack_error_type); // signed, not a uint
    TEST_SIMPLE_READ_ERROR("\x01", (mpack_expect_uint_match(&reader, 2), true), mpack_error_type); // successful uint, not a match

    TEST_SIMPLE_READ("\x00", (mpack_expect_int_match(&reader, 0), true));
    TEST_SIMPLE_READ("\x01", (mpack_expect_int_match(&reader, 1), true));
    TEST_SIMPLE_READ("\xd0\xdf", (mpack_expect_int_match(&reader, -33), true));
    TEST_SIMPLE_READ("\xd0\x80", (mpack_expect_int_match(&reader, INT8_MIN), true));
    TEST_SIMPLE_READ("\xd1\xff\x7f", (mpack_expect_int_match(&reader, INT8_MIN - 1), true));
    TEST_SIMPLE_READ("\xd1\x80\x00", (mpack_expect_int_match(&reader, INT16_MIN), true));
    TEST_SIMPLE_READ("\xd2\xff\xff\x7f\xff", (mpack_expect_int_match(&reader, INT16_MIN - 1), true));
    TEST_SIMPLE_READ("\xd2\x80\x00\x00\x00", (mpack_expect_int_match(&reader, INT32_MIN), true));
    TEST_SIMPLE_READ("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", (mpack_expect_int_match(&reader, (int64_t)INT32_MIN - 1), true));
    TEST_SIMPLE_READ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", (mpack_expect_int_match(&reader, INT64_MIN), true));
    TEST_SIMPLE_READ_ERROR("\xc0", (mpack_expect_int_match(&reader, 0), true), mpack_error_type); // nil, not an int
    TEST_SIMPLE_READ_ERROR("\x01", (mpack_expect_int_match(&reader, 2), true), mpack_error_type); // successful uint->int, not a match
    TEST_SIMPLE_READ_ERROR("\xfe", (mpack_expect_int_match(&reader, -3), true), mpack_error_type); // successful int, not a match

}

static void test_expect_misc() {
    TEST_SIMPLE_READ("\xc0", (mpack_expect_nil(&reader), true));
    TEST_SIMPLE_READ("\xc0", (mpack_expect_tag(&reader, mpack_tag_nil()), true));
    TEST_SIMPLE_READ_ERROR("\x90", (mpack_expect_tag(&reader, mpack_tag_nil()), true), mpack_error_type);

    TEST_SIMPLE_READ("\xc2", false == mpack_expect_bool(&reader));
    TEST_SIMPLE_READ("\xc3", true == mpack_expect_bool(&reader));
    TEST_SIMPLE_READ("\xc2", (mpack_expect_false(&reader), true));
    TEST_SIMPLE_READ("\xc3", (mpack_expect_true(&reader), true));
    TEST_SIMPLE_READ_ERROR("\xc3", (mpack_expect_false(&reader), true), mpack_error_type); // bool, wrong value
    TEST_SIMPLE_READ_ERROR("\xc2", (mpack_expect_true(&reader), true), mpack_error_type); // bool, wrong value
    TEST_SIMPLE_READ_ERROR("\xc0", (mpack_expect_false(&reader), true), mpack_error_type); // wrong type
    TEST_SIMPLE_READ_ERROR("\xc0", (mpack_expect_true(&reader), true), mpack_error_type); // wrong type
}

#if MPACK_READ_TRACKING
static void test_expect_tracking() {
    char buf[4];
    mpack_reader_t reader;

    // tracking depth growth
    TEST_READER_INIT_STR(&reader, "\x91\x91\x91\x91\x91\x91\x90");
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(1 == mpack_expect_array(&reader));
    TEST_TRUE(0 == mpack_expect_array(&reader));
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    mpack_done_array(&reader);
    TEST_READER_DESTROY_NOERROR(&reader);

    // cancel
    TEST_READER_INIT_STR(&reader, "\x90");
    mpack_expect_array(&reader);
    mpack_reader_flag_error(&reader, mpack_error_data);
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_data);

    // done type when nothing was open
    TEST_READER_INIT_STR(&reader, "\x90");
    TEST_BREAK((mpack_done_map(&reader), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // closing incomplete type
    TEST_READER_INIT_STR(&reader, "\x91\xc0");
    mpack_expect_array(&reader);
    TEST_BREAK((mpack_done_array(&reader), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // reading elements in a string
    TEST_READER_INIT_STR(&reader, "\xa2""xx");
    mpack_expect_str(&reader);
    TEST_BREAK((mpack_read_tag(&reader), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // reading too many elements
    TEST_READER_INIT_STR(&reader, "\x90");
    mpack_expect_array(&reader);
    TEST_BREAK((mpack_read_tag(&reader), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // reading bytes with nothing open
    TEST_READER_INIT_STR(&reader, "\x90");
    TEST_BREAK((mpack_read_bytes(&reader, buf, sizeof(buf)), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // reading bytes in an array
    TEST_READER_INIT_STR(&reader, "\x90");
    mpack_expect_array(&reader);
    TEST_BREAK((mpack_read_bytes(&reader, buf, sizeof(buf)), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // reading too many bytes
    TEST_READER_INIT_STR(&reader, "\xa2""xx");
    mpack_expect_str(&reader);
    TEST_BREAK((mpack_read_bytes(&reader, buf, sizeof(buf)), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);

    // checking remaining bytes with unclosed type
    TEST_READER_INIT_STR(&reader, "\xa2""xx");
    mpack_expect_str(&reader);
    TEST_BREAK((mpack_reader_remaining(&reader, NULL), true));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_bug);
}
#endif

static void test_expect_reals() {
    // these are some very simple floats that don't really test IEEE 742 conformance;
    // this section could use some improvement

    TEST_SIMPLE_READ("\x00", 0.0f == mpack_expect_float(&reader));
    TEST_SIMPLE_READ("\xd0\x00", 0.0f == mpack_expect_float(&reader));
    TEST_SIMPLE_READ("\xca\x00\x00\x00\x00", 0.0f == mpack_expect_float(&reader));
    TEST_SIMPLE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_expect_float(&reader));

    TEST_SIMPLE_READ("\x00", 0.0 == mpack_expect_double(&reader));
    TEST_SIMPLE_READ("\xd0\x00", 0.0 == mpack_expect_double(&reader));
    TEST_SIMPLE_READ("\xca\x00\x00\x00\x00", 0.0 == mpack_expect_double(&reader));
    TEST_SIMPLE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0 == mpack_expect_double(&reader));

    TEST_SIMPLE_READ("\xca\x00\x00\x00\x00", 0.0f == mpack_expect_float_strict(&reader));
    TEST_SIMPLE_READ("\xca\x00\x00\x00\x00", 0.0 == mpack_expect_double_strict(&reader));
    TEST_SIMPLE_READ("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0 == mpack_expect_double_strict(&reader));

    // when -ffinite-math-only is enabled, isnan() can always return false.
    // TODO: we should probably add at least a reader option to
    // generate an error on non-finite reals.
    #if !MPACK_FINITE_MATH
    TEST_SIMPLE_READ("\xca\xff\xff\xff\xff", isnanf(mpack_expect_float(&reader)) != 0);
    TEST_SIMPLE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnanf(mpack_expect_float(&reader)) != 0);
    TEST_SIMPLE_READ("\xca\xff\xff\xff\xff", isnan(mpack_expect_double(&reader)) != 0);
    TEST_SIMPLE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_expect_double(&reader)) != 0);
    TEST_SIMPLE_READ("\xca\xff\xff\xff\xff", 0 != isnanf(mpack_expect_float_strict(&reader)));
    TEST_SIMPLE_READ("\xca\xff\xff\xff\xff", 0 != isnan(mpack_expect_double_strict(&reader)));
    TEST_SIMPLE_READ("\xcb\xff\xff\xff\xff\xff\xff\xff\xff", isnan(mpack_expect_double_strict(&reader)));
    #endif

    TEST_SIMPLE_READ_ERROR("\x00", 0.0f == mpack_expect_float_strict(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd0\x00", 0.0f == mpack_expect_float_strict(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 0.0f == mpack_expect_float_strict(&reader), mpack_error_type);

    TEST_SIMPLE_READ_ERROR("\x00", 0.0 == mpack_expect_double_strict(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xd0\x00", 0.0 == mpack_expect_double_strict(&reader), mpack_error_type);
}

static void test_expect_reals_range() {
    TEST_SIMPLE_READ("\x00", 0.0f == mpack_expect_float_range(&reader, 0.0f, 0.0f));
    TEST_SIMPLE_READ("\x00", 0.0f == mpack_expect_float_range(&reader, 0.0f, 1.0f));
    TEST_SIMPLE_READ("\x00", 0.0f == mpack_expect_float_range(&reader, -1.0f, 0.0f));
    TEST_SIMPLE_READ_ERROR("\x00", 1.0f == mpack_expect_float_range(&reader, 1.0f, 2.0f), mpack_error_type);
    TEST_SIMPLE_READ_ASSERT("\x00", mpack_expect_float_range(reader, 1.0f, -1.0f));

    TEST_SIMPLE_READ("\x00", 0.0 == mpack_expect_double_range(&reader, 0.0, 0.0f));
    TEST_SIMPLE_READ("\x00", 0.0 == mpack_expect_double_range(&reader, 0.0, 1.0f));
    TEST_SIMPLE_READ("\x00", 0.0 == mpack_expect_double_range(&reader, -1.0, 0.0f));
    TEST_SIMPLE_READ_ERROR("\x00", 1.0 == mpack_expect_double_range(&reader, 1.0, 2.0f), mpack_error_type);
    TEST_SIMPLE_READ_ASSERT("\x00", mpack_expect_double_range(reader, 1.0, -1.0));
}

static void test_expect_bad_type() {
    // test that all reader functions correctly handle badly typed data
    TEST_SIMPLE_READ_ERROR("\xc2", (mpack_expect_nil(&reader), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", false == mpack_expect_bool(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_u8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_u16(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_u32(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_u64(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_i8(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_i16(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_i32(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0 == mpack_expect_i64(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0.0f == mpack_expect_float(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0.0 == mpack_expect_double(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0.0f == mpack_expect_float_strict(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc0", 0.0 == mpack_expect_double_strict(&reader), mpack_error_type);
}

static void test_expect_pre_error() {
    // test that all reader functinvalidns correctly handle pre-existing errors
    TEST_SIMPLE_READ_ERROR("", (mpack_expect_nil(&reader), true), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", false == mpack_expect_bool(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_u8(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_u16(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_u32(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_u64(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_i8(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_i16(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_i32(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0 == mpack_expect_i64(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0.0f == mpack_expect_float(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0.0 == mpack_expect_double(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0.0f == mpack_expect_float_strict(&reader), mpack_error_invalid);
    TEST_SIMPLE_READ_ERROR("", 0.0 == mpack_expect_double_strict(&reader), mpack_error_invalid);
}

static void test_expect_str() {
    char buf[256];
    #ifdef MPACK_MALLOC
    char* test = NULL;
    #endif


    // str

    TEST_SIMPLE_READ("\xa0", 0 == mpack_expect_str(&reader) && (mpack_done_str(&reader), true));
    TEST_SIMPLE_READ_CANCEL("\xbf", 31 == mpack_expect_str(&reader));
    TEST_SIMPLE_READ_CANCEL("\xd9\x80", 128 == mpack_expect_str(&reader)); // TODO: test str8 compatibility
    TEST_SIMPLE_READ_CANCEL("\xda\x80\x80", 0x8080 == mpack_expect_str(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdb\xff\xff\xff\xff", 0xffffffff == mpack_expect_str(&reader));

    TEST_SIMPLE_READ("\xa0", 0 == mpack_expect_str_buf(&reader, buf, 0));
    TEST_SIMPLE_READ("\xa0", 0 == mpack_expect_str_buf(&reader, buf, 4));
    TEST_SIMPLE_READ("\xa4test", 4 == mpack_expect_str_buf(&reader, buf, 4));
    TEST_SIMPLE_READ_ERROR("\xa5hello", 0 == mpack_expect_str_buf(&reader, buf, 4), mpack_error_too_big);
    TEST_SIMPLE_READ_ERROR("\xa8test", 0 == mpack_expect_str_buf(&reader, buf, sizeof(buf)), mpack_error_invalid);
    TEST_SIMPLE_READ("\xa1\x00", 1 == mpack_expect_str_buf(&reader, buf, 4));

    TEST_SIMPLE_READ("\xa0", (mpack_expect_str_length(&reader, 0), mpack_done_str(&reader), true));
    TEST_SIMPLE_READ_ERROR("\xa0", (mpack_expect_str_length(&reader, 4), true), mpack_error_type);
    TEST_SIMPLE_READ_CANCEL("\xa4", (mpack_expect_str_length(&reader, 4), true));
    TEST_SIMPLE_READ_ERROR("\xa5", (mpack_expect_str_length(&reader, 4), true), mpack_error_type);

    // cstr
    TEST_SIMPLE_READ_ASSERT("\xa0", mpack_expect_cstr(reader, buf, 0));
    TEST_SIMPLE_READ("\xa0", (mpack_expect_cstr(&reader, buf, 4), true));
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_READ("\xa4test", (mpack_expect_cstr(&reader, buf, 5), true));
    TEST_TRUE(strlen(buf) == 4);
    TEST_SIMPLE_READ_ERROR("\xa5hello", (mpack_expect_cstr(&reader, buf, 5), true), mpack_error_too_big);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_READ("\xa5hello", (mpack_expect_cstr(&reader, buf, sizeof(buf)), true));
    TEST_TRUE(strlen(buf) == 5);
    TEST_SIMPLE_READ_ERROR("\xa5he\x0lo", (mpack_expect_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);

    #ifdef MPACK_MALLOC
    // cstr alloc
    TEST_SIMPLE_READ_BREAK("\xa0", NULL == mpack_expect_cstr_alloc(&reader, 0));
    TEST_SIMPLE_READ("\xa0", NULL != (test = mpack_expect_cstr_alloc(&reader, 4)));
    if (test) {
        TEST_TRUE(strlen(test) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ_ERROR("\xa4test", NULL == mpack_expect_cstr_alloc(&reader, 4), mpack_error_too_big);
    TEST_SIMPLE_READ("\xa4test", NULL != (test = mpack_expect_cstr_alloc(&reader, 5)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ("\xa4test", NULL != (test = mpack_expect_cstr_alloc(&reader, SIZE_MAX)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ_ERROR("\xa5he\00lo", NULL == mpack_expect_cstr_alloc(&reader, 256), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\x01", NULL == mpack_expect_cstr_alloc(&reader, 3), mpack_error_type);
    #endif

    // cstr match
    TEST_SIMPLE_READ("\xa0", (mpack_expect_cstr_match(&reader, ""), true));
    TEST_SIMPLE_READ("\xa3""abc", (mpack_expect_cstr_match(&reader, "abc"), true));
    TEST_SIMPLE_READ_ERROR("\xa0", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xa3""abc", (mpack_expect_cstr_match(&reader, ""), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xa3""zbc", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xa3""azc", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xa3""abz", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);

    #if MPACK_STDLIB
    // str/cstr match larger than 32 bits
    if (UINT32_MAX < SIZE_MAX) {
        test_system_mock_strlen((size_t)UINT32_MAX + (size_t)1);
        TEST_SIMPLE_READ_ERROR("\xa3""abc", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);
        test_system_mock_strlen(SIZE_MAX);
        TEST_SIMPLE_READ_ERROR("\xa3""abc", (mpack_expect_cstr_match(&reader, "abc"), true), mpack_error_type);
    }
    #endif



    // bin is never allowed to be read as str

    TEST_SIMPLE_READ_ERROR("\xc4\x10", 0 == mpack_expect_str(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xc4\x10", (mpack_expect_str_buf(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_READ_ERROR("\xc4\x10", (mpack_expect_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_TRUE(strlen(buf) == 0);


    // utf-8

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

    // utf8 str
    TEST_SIMPLE_READ("\xa0", 0 == mpack_expect_utf8(&reader, buf, 0));
    TEST_SIMPLE_READ("\xa0", 0 == mpack_expect_utf8(&reader, buf, 4));
    TEST_SIMPLE_READ("\xa4test", 4 == mpack_expect_utf8(&reader, buf, 4));
    TEST_SIMPLE_READ_ERROR("\xa5hello", 0 == mpack_expect_utf8(&reader, buf, 4), mpack_error_too_big);
    TEST_SIMPLE_READ(utf8_null, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true));
    TEST_SIMPLE_READ(utf8_valid, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true));
    TEST_SIMPLE_READ(utf8_trimmed, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true));
    TEST_SIMPLE_READ_ERROR(utf8_invalid, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_invalid_trimmed, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_truncated, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_modified, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_cesu8, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_wobbly, (mpack_expect_utf8(&reader, buf, sizeof(buf)), true), mpack_error_type);

    // utf8 cstr
    TEST_SIMPLE_READ_ASSERT("\xa0", mpack_expect_utf8_cstr(reader, buf, 0));
    TEST_SIMPLE_READ("\xa0", (mpack_expect_utf8_cstr(&reader, buf, 4), true));
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_READ("\xa4test", (mpack_expect_utf8_cstr(&reader, buf, 5), true));
    TEST_TRUE(strlen(buf) == 4);
    TEST_SIMPLE_READ_ERROR("\xa5hello", (mpack_expect_utf8_cstr(&reader, buf, 5), true), mpack_error_too_big);
    TEST_TRUE(strlen(buf) == 0);
    TEST_SIMPLE_READ_ERROR(utf8_null, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ(utf8_valid, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true));
    TEST_SIMPLE_READ(utf8_trimmed, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true));
    TEST_SIMPLE_READ_ERROR(utf8_invalid, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_invalid_trimmed, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_truncated, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_modified, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_cesu8, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_wobbly, (mpack_expect_utf8_cstr(&reader, buf, sizeof(buf)), true), mpack_error_type);

    #ifdef MPACK_MALLOC
    // utf8 cstr alloc
    TEST_SIMPLE_READ_BREAK("\xa0", NULL == mpack_expect_utf8_cstr_alloc(&reader, 0));
    TEST_SIMPLE_READ("\xa0", NULL != (test = mpack_expect_utf8_cstr_alloc(&reader, 4)));
    if (test) {
        TEST_TRUE(strlen(test) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ_ERROR("\xa4test", NULL == mpack_expect_utf8_cstr_alloc(&reader, 4), mpack_error_too_big);
    TEST_SIMPLE_READ("\xa4test", NULL != (test = mpack_expect_utf8_cstr_alloc(&reader, 5)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ("\xa4test", NULL != (test = mpack_expect_utf8_cstr_alloc(&reader, SIZE_MAX)));
    if (test) {
        TEST_TRUE(strlen(test) == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }
    TEST_SIMPLE_READ_ERROR("\x01", NULL == mpack_expect_utf8_cstr_alloc(&reader, 3), mpack_error_type);

    TEST_SIMPLE_READ_ERROR(utf8_null, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ(utf8_valid, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true));
    MPACK_FREE(test);
    TEST_SIMPLE_READ(utf8_trimmed, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true));
    MPACK_FREE(test);
    TEST_SIMPLE_READ_ERROR(utf8_invalid, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_invalid_trimmed, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_truncated, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_modified, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_cesu8, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    TEST_SIMPLE_READ_ERROR(utf8_wobbly, ((test = mpack_expect_utf8_cstr_alloc(&reader, 256)), true), mpack_error_type);
    #endif

}

static void test_expect_bin() {
    char buf[256];

    TEST_SIMPLE_READ_CANCEL("\xc4\x80", 128 == mpack_expect_bin(&reader));
    TEST_SIMPLE_READ_CANCEL("\xc5\x80\x80", 0x8080 == mpack_expect_bin(&reader));
    TEST_SIMPLE_READ_CANCEL("\xc6\xff\xff\xff\xff", 0xffffffff == mpack_expect_bin(&reader));

    // TODO: test strict/compatibility modes. currently, we do not
    // support old MessagePack version compatibility; bin will not
    // accept str types.
    TEST_SIMPLE_READ_ERROR("\xbf", 0 == mpack_expect_bin(&reader), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\xbf", 0 == mpack_expect_bin_buf(&reader, buf, sizeof(buf)), mpack_error_type);

    TEST_SIMPLE_READ("\xc4\x00", 0 == mpack_expect_bin_buf(&reader, buf, 0));
    TEST_SIMPLE_READ("\xc4\x00", 0 == mpack_expect_bin_buf(&reader, buf, 4));
    TEST_SIMPLE_READ("\xc4\x04test", 4 == mpack_expect_bin_buf(&reader, buf, 4));
    TEST_SIMPLE_READ_ERROR("\xc4\x05hello", 0 == mpack_expect_bin_buf(&reader, buf, 4), mpack_error_too_big);
    TEST_SIMPLE_READ_ERROR("\xc4\x08hello", 0 == mpack_expect_bin_buf(&reader, buf, sizeof(buf)), mpack_error_invalid);
    TEST_SIMPLE_READ("\xc4\x01\x00", 1 == mpack_expect_bin_buf(&reader, buf, 4));

    TEST_SIMPLE_READ("\xc4\x00", (mpack_expect_bin_size(&reader, 0), mpack_done_bin(&reader), true));
    TEST_SIMPLE_READ_ERROR("\xc4\x00", (mpack_expect_bin_size(&reader, 4), true), mpack_error_type);
    TEST_SIMPLE_READ_CANCEL("\xc4\x04", (mpack_expect_bin_size(&reader, 4), true));
    TEST_SIMPLE_READ_ERROR("\xc4\x05", (mpack_expect_bin_size(&reader, 4), true), mpack_error_type);

    #ifdef MPACK_MALLOC
    size_t length;
    char* test = NULL;

    TEST_SIMPLE_READ("\xc4\x00", (NULL == mpack_expect_bin_alloc(&reader, 0, &length)));
    TEST_TRUE(length == 0);
    TEST_SIMPLE_READ("\xc4\x00", (NULL == mpack_expect_bin_alloc(&reader, 4, &length)));
    TEST_TRUE(length == 0);
    TEST_SIMPLE_READ("\xc4\x04test", NULL != (test = mpack_expect_bin_alloc(&reader, 4, &length)));
    if (test) {
        TEST_TRUE(length == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }

    // Unlimited max allocation size. Don't do this, or at least not with
    // untrusted data!
    TEST_SIMPLE_READ("\xc4\x04test", NULL != (test = mpack_expect_bin_alloc(&reader, SIZE_MAX, &length)));
    if (test) {
        TEST_TRUE(length == 4);
        TEST_TRUE(memcmp(test, "test", 4) == 0);
        MPACK_FREE(test);
    }

    TEST_SIMPLE_READ_ERROR("\xc4\x04test", NULL == mpack_expect_bin_alloc(&reader, 3, &length), mpack_error_type);
    TEST_SIMPLE_READ_ERROR("\x01", NULL == mpack_expect_bin_alloc(&reader, 3, &length), mpack_error_type);
    #endif

}

static void test_expect_ext() {
    char buf[256];
    int8_t type;

    TEST_SIMPLE_READ_CANCEL("\xd4\x01\x80", 1 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xd5\x01\x80\x80", 2 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xd6\x01\xff\xff\xff\xff", 4 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xd7\x01\xff\xff\xff\xff\xff\xff\xff\xff", 8 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xd8\x01\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", 16 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xc7\x00\x01", 0 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xc7\x01\x01\xff", 1 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xc8\x00\x01\x01\xff", 1 == mpack_expect_ext(&reader, &type));
    TEST_SIMPLE_READ_CANCEL("\xc9\x00\x00\x00\x01\x01\xff", 1 == mpack_expect_ext(&reader, &type));

    // TODO: test strict/compatibility modes. currently, we do not
    // support old MessagePack version compatibility; ext will not
    // accept str types.
    /*TEST_SIMPLE_READ_ERROR("\xbf", 0 == mpack_expect_ext(&reader, &type), mpack_error_type);*/
    /*TEST_SIMPLE_READ_ERROR("\xbf", 0 == mpack_expect_ext_buf(&reader, buf, sizeof(buf)), mpack_error_type);*/

    TEST_SIMPLE_READ("\xd4\x01\x00", 1 == mpack_expect_ext_buf(&reader, &type, buf, 1));
    TEST_SIMPLE_READ("\xd5\x01te", 2 == mpack_expect_ext_buf(&reader, &type, buf, 2));
    TEST_SIMPLE_READ("\xd6\x01test", 4 == mpack_expect_ext_buf(&reader, &type, buf, 4));
    /*TEST_SIMPLE_READ_ERROR("\xc4\x05hello", 0 == mpack_expect_ext_buf(&reader, buf, 4), mpack_error_too_big);*/
    /*TEST_SIMPLE_READ_ERROR("\xc4\x08hello", 0 == mpack_expect_ext_buf(&reader, buf, sizeof(buf)), mpack_error_invalid);*/
    /*TEST_SIMPLE_READ("\xc4\x01\x00", 1 == mpack_expect_ext_buf(&reader, buf, 4));*/

    /*TEST_SIMPLE_READ("\xc4\x00", (mpack_expect_ext_size(&reader, 0), mpack_done_ext(&reader), true));*/
    /*TEST_SIMPLE_READ_ERROR("\xc4\x00", (mpack_expect_ext_size(&reader, 4), true), mpack_error_type);*/
    /*TEST_SIMPLE_READ_CANCEL("\xc4\x04", (mpack_expect_ext_size(&reader, 4), true));*/
    /*TEST_SIMPLE_READ_ERROR("\xc4\x05", (mpack_expect_ext_size(&reader, 4), true), mpack_error_type);*/

    /*#ifdef MPACK_MALLOC*/
    /*size_t length;*/
    /*char* test = NULL;*/

    /*TEST_SIMPLE_READ("\xc4\x00", (NULL == mpack_expect_ext_alloc(&reader, 0, &length)));*/
    /*TEST_TRUE(length == 0);*/
    /*TEST_SIMPLE_READ("\xc4\x00", (NULL == mpack_expect_ext_alloc(&reader, 4, &length)));*/
    /*TEST_TRUE(length == 0);*/
    /*TEST_SIMPLE_READ("\xc4\x04test", NULL != (test = mpack_expect_ext_alloc(&reader, 4, &length)));*/
    /*if (test) {*/
        /*TEST_TRUE(length == 4);*/
        /*TEST_TRUE(memcmp(test, "test", 4) == 0);*/
        /*MPACK_FREE(test);*/
    /*}*/

    /*// Unlimited max allocation size. Don't do this, or at least not with*/
    /*// untrusted data!*/
    /*TEST_SIMPLE_READ("\xc4\x04test", NULL != (test = mpack_expect_ext_alloc(&reader, SIZE_MAX, &length)));*/
    /*if (test) {*/
        /*TEST_TRUE(length == 4);*/
        /*TEST_TRUE(memcmp(test, "test", 4) == 0);*/
        /*MPACK_FREE(test);*/
    /*}*/

    /*TEST_SIMPLE_READ_ERROR("\xc4\x04test", NULL == mpack_expect_ext_alloc(&reader, 3, &length), mpack_error_type);*/
    /*TEST_SIMPLE_READ_ERROR("\x01", NULL == mpack_expect_ext_alloc(&reader, 3, &length), mpack_error_type);*/
    /*#endif*/
}

static void test_expect_arrays() {
    uint32_t count;

    // arrays

    TEST_SIMPLE_READ_CANCEL("\x90", 0 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\x91", 1 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\x9f", 15 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdc\x00\x00", 0 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdc\x01\x00", 0x100 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdc\xff\xff", 0xffff == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdd\x00\x00\x00\x00", 0 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdd\x00\x00\x01\x00", 0x100 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdd\x00\x01\x00\x00", 0x10000 == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdd\xff\xff\xff\xff", UINT32_MAX == mpack_expect_array(&reader));
    TEST_SIMPLE_READ_ERROR("\x00", 0 == mpack_expect_array(&reader), mpack_error_type);

    // array ranges

    TEST_SIMPLE_READ_CANCEL("\x91", 1 == mpack_expect_array_range(&reader, 0, 1));
    TEST_SIMPLE_READ_CANCEL("\x91", 1 == mpack_expect_array_range(&reader, 1, 1));
    TEST_SIMPLE_READ_ERROR("\x91", 2 == mpack_expect_array_range(&reader, 2, 2), mpack_error_type);
    TEST_SIMPLE_READ_ASSERT("\x91", mpack_expect_array_range(reader, 2, 1));
    TEST_SIMPLE_READ_CANCEL("\x91", 1 == mpack_expect_array_max(&reader, 1));
    TEST_SIMPLE_READ_ERROR("\x91", 0 == mpack_expect_array_max(&reader, 0), mpack_error_type);

    TEST_SIMPLE_READ("\x90", (mpack_expect_array_match(&reader, 0), mpack_done_array(&reader), true));
    TEST_SIMPLE_READ_CANCEL("\x9f", (mpack_expect_array_match(&reader, 15), true));
    TEST_SIMPLE_READ_CANCEL("\xdc\xff\xff", (mpack_expect_array_match(&reader, 0xffff), true));
    TEST_SIMPLE_READ_CANCEL("\xdd\xff\xff\xff\xff", (mpack_expect_array_match(&reader, UINT32_MAX), true));
    TEST_SIMPLE_READ_ERROR("\x91", (mpack_expect_array_match(&reader, 2), true), mpack_error_type);

    TEST_SIMPLE_READ_CANCEL("\x91", true == mpack_expect_array_or_nil(&reader, &count));
    TEST_TRUE(count == 1);
    TEST_SIMPLE_READ_CANCEL("\xc0", false == mpack_expect_array_or_nil(&reader, &count));
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x81", false == mpack_expect_array_or_nil(&reader, &count), mpack_error_type);
    TEST_TRUE(count == 0);

    TEST_SIMPLE_READ_CANCEL("\x91", true == mpack_expect_array_max_or_nil(&reader, 1, &count));
    TEST_TRUE(count == 1);
    TEST_SIMPLE_READ_CANCEL("\xc0", false == mpack_expect_array_max_or_nil(&reader, 0, &count));
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x92", false == mpack_expect_array_max_or_nil(&reader, 1, &count), mpack_error_type);
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x81", false == mpack_expect_array_max_or_nil(&reader, 1, &count), mpack_error_type);
    TEST_TRUE(count == 0);

    // array allocs

    #ifdef MPACK_MALLOC
    int* elements;

    TEST_SIMPLE_READ("\x90", (elements = mpack_expect_array_alloc(&reader, int, 1, &count), mpack_done_array(&reader), true));
    TEST_TRUE(elements == NULL);
    TEST_SIMPLE_READ_CANCEL("\x91", NULL != (elements = mpack_expect_array_alloc(&reader, int, 1, &count)));
    if (elements) {
        elements[0] = 0;
        MPACK_FREE(elements);
    }
    TEST_SIMPLE_READ_CANCEL("\x92", NULL != (elements = mpack_expect_array_alloc(&reader, int, 2, &count)));
    if (elements) {
        elements[0] = 0;
        elements[1] = 1;
        MPACK_FREE(elements);
    }

    TEST_SIMPLE_READ_ERROR("\x92", (elements = mpack_expect_array_alloc(&reader, int, 1, &count), true), mpack_error_type);
    TEST_TRUE(elements == NULL);
    TEST_SIMPLE_READ_ERROR("\xc0", (elements = mpack_expect_array_alloc(&reader, int, 1, &count), true), mpack_error_type);
    TEST_TRUE(elements == NULL);

    TEST_SIMPLE_READ("\x90", (elements = mpack_expect_array_or_nil_alloc(&reader, int, 1, &count), true));
    TEST_TRUE(elements == NULL);
    TEST_SIMPLE_READ_CANCEL("\x91", NULL != (elements = mpack_expect_array_or_nil_alloc(&reader, int, 1, &count)));
    if (elements) {
        elements[0] = 0;
        MPACK_FREE(elements);
    }
    TEST_SIMPLE_READ_CANCEL("\x92", NULL != (elements = mpack_expect_array_or_nil_alloc(&reader, int, 2, &count)));
    if (elements) {
        elements[0] = 0;
        elements[1] = 1;
        MPACK_FREE(elements);
    }

    TEST_SIMPLE_READ_ERROR("\x92", (elements = mpack_expect_array_or_nil_alloc(&reader, int, 1, &count), true), mpack_error_type);
    TEST_TRUE(elements == NULL);
    TEST_SIMPLE_READ("\xc0", (elements = mpack_expect_array_or_nil_alloc(&reader, int, 1, &count), true));
    TEST_TRUE(elements == NULL);
    #endif

}

static void test_expect_maps() {
    uint32_t count;

    // maps

    TEST_SIMPLE_READ_CANCEL("\x80", 0 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\x81", 1 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\x8f", 15 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xde\x00\x00", 0 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xde\x01\x00", 0x100 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xde\xff\xff", 0xffff == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdf\x00\x00\x00\x00", 0 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdf\x00\x00\x01\x00", 0x100 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdf\x00\x01\x00\x00", 0x10000 == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_CANCEL("\xdf\xff\xff\xff\xff", UINT32_MAX == mpack_expect_map(&reader));
    TEST_SIMPLE_READ_ERROR("\x00", 0 == mpack_expect_map(&reader), mpack_error_type);

    // map ranges

    TEST_SIMPLE_READ_CANCEL("\x81", 1 == mpack_expect_map_range(&reader, 0, 1));
    TEST_SIMPLE_READ_CANCEL("\x81", 1 == mpack_expect_map_range(&reader, 1, 1));
    TEST_SIMPLE_READ_ERROR("\x81", 2 == mpack_expect_map_range(&reader, 2, 2), mpack_error_type);
    TEST_SIMPLE_READ_ASSERT("\x81", mpack_expect_map_range(reader, 2, 1));
    TEST_SIMPLE_READ_CANCEL("\x81", 1 == mpack_expect_map_max(&reader, 1));
    TEST_SIMPLE_READ_ERROR("\x81", 0 == mpack_expect_map_max(&reader, 0), mpack_error_type);

    TEST_SIMPLE_READ("\x80", (mpack_expect_map_match(&reader, 0), mpack_done_map(&reader), true));
    TEST_SIMPLE_READ_CANCEL("\x8f", (mpack_expect_map_match(&reader, 15), true));
    TEST_SIMPLE_READ_CANCEL("\xde\xff\xff", (mpack_expect_map_match(&reader, 0xffff), true));
    TEST_SIMPLE_READ_CANCEL("\xdf\xff\xff\xff\xff", (mpack_expect_map_match(&reader, UINT32_MAX), true));
    TEST_SIMPLE_READ_ERROR("\x81", (mpack_expect_map_match(&reader, 2), true), mpack_error_type);

    TEST_SIMPLE_READ_CANCEL("\x81", true == mpack_expect_map_or_nil(&reader, &count));
    TEST_TRUE(count == 1);
    TEST_SIMPLE_READ_CANCEL("\xc0", false == mpack_expect_map_or_nil(&reader, &count));
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x91", false == mpack_expect_map_or_nil(&reader, &count), mpack_error_type);
    TEST_TRUE(count == 0);

    TEST_SIMPLE_READ_CANCEL("\x81", true == mpack_expect_map_max_or_nil(&reader, 1, &count));
    TEST_TRUE(count == 1);
    TEST_SIMPLE_READ_CANCEL("\xc0", false == mpack_expect_map_max_or_nil(&reader, 0, &count));
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x82", false == mpack_expect_map_max_or_nil(&reader, 1, &count), mpack_error_type);
    TEST_TRUE(count == 0);
    TEST_SIMPLE_READ_ERROR("\x91", false == mpack_expect_map_max_or_nil(&reader, 1, &count), mpack_error_type);
    TEST_TRUE(count == 0);

}

static void test_expect_key_cstr_basic() {
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, test_example, TEST_EXAMPLE_SIZE);

    static const char* keys[] = {"schema","compact"};
    #define KEY_COUNT (sizeof(keys) / sizeof(keys[0]))
    bool found[KEY_COUNT];
    memset(found, 0, sizeof(found));

    TEST_TRUE(2 == mpack_expect_map(&reader));
    TEST_TRUE(1 == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT));
    TEST_TRUE(true == mpack_expect_bool(&reader));
    TEST_TRUE(0 == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT));
    TEST_TRUE(0 == mpack_expect_u8(&reader));
    mpack_done_map(&reader);

    TEST_READER_DESTROY_NOERROR(&reader);
    TEST_TRUE(found[0]);
    TEST_TRUE(found[1]);
    #undef KEY_COUNT
}

static void test_expect_key_cstr_mixed() {
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, test_example, TEST_EXAMPLE_SIZE);

    static const char* keys[] = { "unknown", "schema", "unknown2" };
    #define KEY_COUNT (sizeof(keys) / sizeof(keys[0]))
    bool found[KEY_COUNT];
    memset(found, 0, sizeof(found));

    TEST_TRUE(2 == mpack_expect_map(&reader));
    TEST_TRUE(KEY_COUNT == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT)); // unknown
    mpack_discard(&reader);
    TEST_TRUE(1 == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT));
    TEST_TRUE(0 == mpack_expect_u8(&reader));
    mpack_done_map(&reader);

    TEST_READER_DESTROY_NOERROR(&reader);
    TEST_TRUE(!found[0]);
    TEST_TRUE(found[1]);
    TEST_TRUE(!found[2]);
    #undef KEY_COUNT
}

static void test_expect_key_cstr_duplicate() {
    static const char data[] = "\x83\xA3""dup\xC0\xA3""dup\xC0\xA5""valid\xC0";
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, sizeof(data)-1);

    static const char* keys[] = { "valid", "dup" };
    #define KEY_COUNT (sizeof(keys) / sizeof(keys[0]))
    bool found[KEY_COUNT];
    memset(found, 0, sizeof(found));

    TEST_TRUE(3 == mpack_expect_map(&reader));
    TEST_TRUE(1 == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT));
    mpack_expect_nil(&reader);
    TEST_TRUE(KEY_COUNT == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT)); // duplicate
    mpack_discard(&reader); // should be no-op due to error
    TEST_TRUE(KEY_COUNT == mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT)); // already in error, not valid

    TEST_READER_DESTROY_ERROR(&reader, mpack_error_invalid);
    #undef KEY_COUNT
}

static void test_expect_key_uint() {
    static const char data[] = "\x85\x02\xC0\x00\xC0\xC3\xC0\x03\xC0\x03\xC0";
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, sizeof(data)-1);

    #define KEY_COUNT 4
    bool found[KEY_COUNT];
    memset(found, 0, sizeof(found));

    TEST_TRUE(5 == mpack_expect_map(&reader));
    TEST_TRUE(2 == mpack_expect_key_uint(&reader, found, KEY_COUNT));
    mpack_expect_nil(&reader);
    TEST_TRUE(0 == mpack_expect_key_uint(&reader, found, KEY_COUNT));
    mpack_expect_nil(&reader);
    TEST_TRUE(KEY_COUNT == mpack_expect_key_uint(&reader, found, KEY_COUNT)); // key has value "true", unrecognized
    mpack_discard(&reader);
    TEST_TRUE(3 == mpack_expect_key_uint(&reader, found, KEY_COUNT));
    mpack_expect_nil(&reader);

    TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);
    TEST_TRUE(found[0]);
    TEST_TRUE(!found[1]);
    TEST_TRUE(found[2]);
    TEST_TRUE(found[2]);

    TEST_TRUE(KEY_COUNT == mpack_expect_key_uint(&reader, found, KEY_COUNT));
    TEST_READER_DESTROY_ERROR(&reader, mpack_error_invalid);
    #undef KEY_COUNT
}

typedef struct test_expect_stream_t {
    char* data;
    size_t left;
    size_t read_size;
} test_expect_stream_t;

static size_t test_expect_stream_fill(mpack_reader_t* reader, char* buffer, size_t count) {
    test_expect_stream_t* context = (test_expect_stream_t*)reader->context;
    if (count > context->read_size)
        count = context->read_size;
    if (count > context->left)
        count = context->left;
    memcpy(buffer, context->data, count);
    context->data += count;
    context->left -= count;
    return count;
}

static void test_expect_streaming() {
    // We test reading from a stream of messages using a function
    // that returns a small number of bytes each time (as though
    // it is slowly receiving data through a socket.) This tests
    // that the reader correctly handles streams, and that it
    // can continue asking for data even when it needs more bytes
    // than read by a single call to the fill function.

    char data[] = "\x00\xd3\xff\xff\xff\xff\xff\xff\xff\xff\xc0\xa5"
        "hello\x93\xc3\xa6""world!\xc2\xce\xff\xff\xff\xff";

    size_t sizes[] = {1, 2, 3, 5, 7, 11};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {

        test_expect_stream_t context = {data, sizeof(data) - 1, sizes[i]};
        mpack_reader_t reader;
        char buffer[MPACK_READER_MINIMUM_BUFFER_SIZE];
        mpack_reader_init(&reader, buffer, sizeof(buffer), 0);
        mpack_reader_set_context(&reader, &context);
        mpack_reader_set_fill(&reader, &test_expect_stream_fill);

        TEST_TRUE(mpack_expect_uint(&reader) == 0);
        TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);
        TEST_TRUE(mpack_expect_i64(&reader) == -1);
        mpack_expect_nil(&reader);
        TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);
        mpack_expect_cstr_match(&reader, "hello");
        TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);

        TEST_TRUE(mpack_expect_array(&reader) == 3);
        TEST_TRUE(mpack_expect_bool(&reader) == true);
        mpack_expect_cstr_match(&reader, "world!");
        TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);
        mpack_expect_false(&reader);
        TEST_TRUE(mpack_reader_error(&reader) == mpack_ok);
        mpack_done_array(&reader);

        TEST_TRUE(mpack_expect_u32(&reader) == UINT32_MAX);
        TEST_READER_DESTROY_NOERROR(&reader);
    }
}

void test_expect() {
    test_expect_example_read();

    // int/uint
    test_expect_uint_fixnum();
    test_expect_uint_signed_fixnum();
    test_expect_negative_fixnum();
    test_expect_uint();
    test_expect_uint_signed();
    test_expect_int();
    test_expect_uint_bounds();
    test_expect_int_bounds();
    test_expect_ints_dynamic_int();
    test_expect_int_range();
    test_expect_int_match();

    // compound types
    test_expect_str();
    test_expect_bin();
    test_expect_ext();
    test_expect_arrays();
    test_expect_maps();

    // key switches
    test_expect_key_cstr_basic();
    test_expect_key_cstr_mixed();
    test_expect_key_cstr_duplicate();
    test_expect_key_uint();

    // other
    test_expect_misc();
    #if MPACK_READ_TRACKING
    test_expect_tracking();
    #endif
    test_expect_reals();
    test_expect_reals_range();
    test_expect_bad_type();
    test_expect_pre_error();
    test_expect_streaming();
}

#endif

