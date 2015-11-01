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

#include "test-write.h"
#include "test.h"

#if MPACK_WRITER

mpack_error_t test_write_error = mpack_ok;

void test_write_error_handler(mpack_writer_t* writer, mpack_error_t error) {
    TEST_TRUE(test_write_error == mpack_ok, "error handler was called multiple times");
    TEST_TRUE(error != mpack_ok, "error handler was called with mpack_ok");
    TEST_TRUE(mpack_writer_error(writer) == error, "writer error does not match given error");
    test_write_error = error;
}

// writes ints using the auto int()/uint() functions
static void test_write_simple_auto_int() {
    char buf[4096];

    // positive fixnums
    TEST_SIMPLE_WRITE("\x00", mpack_write_uint(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_uint(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_uint(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_uint(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_uint(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_uint(&writer, 0x7f));

    // positive fixnums with signed int functions
    TEST_SIMPLE_WRITE("\x00", mpack_write_int(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_int(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_int(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_int(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_int(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_int(&writer, 0x7f));

    // negative fixnums
    TEST_SIMPLE_WRITE("\xff", mpack_write_int(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_int(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_int(&writer, -16));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_int(&writer, -32));

    // uints
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_uint(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_uint(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_uint(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_uint(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_uint(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_uint(&writer, 0xffffffff));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_uint(&writer, UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_uint(&writer, UINT64_C(0xffffffffffffffff)));

    // positive ints with signed value
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_int(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_int(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_int(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_int(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_int(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_int(&writer, INT64_C(0xffffffff)));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_int(&writer, INT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_int(&writer, INT64_C(0x7fffffffffffffff)));

    // ints
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_int(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_int(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_int(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_int(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_int(&writer, -32769));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_int(&writer, INT64_C(-2147483648)));

    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_int(&writer, INT64_C(-2147483649)));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_int(&writer, INT64_MIN));

}

// writes ints using the sized iXX()/uXX() functions
static void test_write_simple_size_int_fixnums() {
    char buf[4096];

    // positive fixnums
    TEST_SIMPLE_WRITE("\x00", mpack_write_u8(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_u8(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_u8(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_u8(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_u8(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_u8(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_u16(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_u16(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_u16(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_u16(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_u16(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_u16(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_u32(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_u32(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_u32(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_u32(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_u32(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_u32(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_u64(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_u64(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_u64(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_u64(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_u64(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_u64(&writer, 0x7f));

    // positive fixnums with signed int functions
    TEST_SIMPLE_WRITE("\x00", mpack_write_i8(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_i8(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_i8(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_i8(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_i8(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_i8(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_i16(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_i16(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_i16(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_i16(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_i16(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_i16(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_i32(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_i32(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_i32(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_i32(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_i32(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_i32(&writer, 0x7f));
    TEST_SIMPLE_WRITE("\x00", mpack_write_i64(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_i64(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_i64(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_i64(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_i64(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_i64(&writer, 0x7f));

    // negative fixnums
    TEST_SIMPLE_WRITE("\xff", mpack_write_i8(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_i8(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_i8(&writer, -16));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_i8(&writer, -32));
    TEST_SIMPLE_WRITE("\xff", mpack_write_i16(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_i16(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_i16(&writer, -16));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_i16(&writer, -32));
    TEST_SIMPLE_WRITE("\xff", mpack_write_i32(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_i32(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_i32(&writer, -16));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_i32(&writer, -32));
    TEST_SIMPLE_WRITE("\xff", mpack_write_i64(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_i64(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_i64(&writer, -16));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_i64(&writer, -32));
}

static void test_write_simple_size_int() {
    char buf[4096];

    // uints
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_u8(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_u8(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_u16(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_u16(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_u16(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_u16(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_u32(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_u32(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_u32(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_u32(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_u32(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_u32(&writer, 0xffffffff));
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_u64(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_u64(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_u64(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_u64(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_u64(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_u64(&writer, 0xffffffff));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_u64(&writer, UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_u64(&writer, UINT64_C(0xffffffffffffffff)));

    // positive ints with signed value
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_i16(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_i16(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_i16(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write_i16(&writer, 0x7fff));
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_i32(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_i32(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_i32(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write_i32(&writer, 0x7fff));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_i32(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_i32(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\x7f\xff\xff\xff", mpack_write_i32(&writer, 0x7fffffff));
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_i64(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_i64(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_i64(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write_i64(&writer, 0x7fff));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_i64(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_i64(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\x7f\xff\xff\xff", mpack_write_i64(&writer, 0x7fffffff));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_i64(&writer, INT64_C(0xffffffff)));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_i64(&writer, UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_i64(&writer, UINT64_C(0x7fffffffffffffff)));

    // negative ints
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_i8(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_i8(&writer, -128));
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_i16(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_i16(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_i16(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_i16(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_i32(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_i32(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_i32(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_i32(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_i32(&writer, -32769));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_i32(&writer, INT64_C(-2147483648)));
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_i64(&writer, INT64_C(-2147483648)));

    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_i64(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_i64(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_i64(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_i64(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_i64(&writer, -32769));
    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_i64(&writer, INT64_C(-2147483649)));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_i64(&writer, INT64_MIN));

}

// writes ints using the dynamic tag writer function
static void test_write_simple_tag_int() {
    char buf[4096];

    // positive fixnums
    TEST_SIMPLE_WRITE("\x00", mpack_write_tag(&writer, mpack_tag_uint(0)));
    TEST_SIMPLE_WRITE("\x01", mpack_write_tag(&writer, mpack_tag_uint(1)));
    TEST_SIMPLE_WRITE("\x02", mpack_write_tag(&writer, mpack_tag_uint(2)));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_tag(&writer, mpack_tag_uint(0x0f)));
    TEST_SIMPLE_WRITE("\x10", mpack_write_tag(&writer, mpack_tag_uint(0x10)));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_tag(&writer, mpack_tag_uint(0x7f)));

    // positive fixnums with signed value
    TEST_SIMPLE_WRITE("\x00", mpack_write_tag(&writer, mpack_tag_int(0)));
    TEST_SIMPLE_WRITE("\x01", mpack_write_tag(&writer, mpack_tag_int(1)));
    TEST_SIMPLE_WRITE("\x02", mpack_write_tag(&writer, mpack_tag_int(2)));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_tag(&writer, mpack_tag_int(0x0f)));
    TEST_SIMPLE_WRITE("\x10", mpack_write_tag(&writer, mpack_tag_int(0x10)));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_tag(&writer, mpack_tag_int(0x7f)));

    // negative fixnums
    TEST_SIMPLE_WRITE("\xff", mpack_write_tag(&writer, mpack_tag_int(-1)));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_tag(&writer, mpack_tag_int(-2)));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_tag(&writer, mpack_tag_int(-16)));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_tag(&writer, mpack_tag_int(-32)));

    // uints
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_tag(&writer, mpack_tag_uint(0x80)));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_tag(&writer, mpack_tag_uint(0xff)));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_tag(&writer, mpack_tag_uint(0x100)));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_tag(&writer, mpack_tag_uint(0xffff)));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_tag(&writer, mpack_tag_uint(0x10000)));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_uint(0xffffffff)));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_uint(UINT64_C(0x100000000))));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_uint(UINT64_C(0xffffffffffffffff))));

    // positive ints with signed value
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_tag(&writer, mpack_tag_int(0x80)));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_tag(&writer, mpack_tag_int(0xff)));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_tag(&writer, mpack_tag_int(0x100)));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_tag(&writer, mpack_tag_int(0xffff)));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_tag(&writer, mpack_tag_int(0x10000)));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(INT64_C(0xffffffff))));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(INT64_C(0x100000000))));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(INT64_C(0x7fffffffffffffff))));

    // ints
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_tag(&writer, mpack_tag_int(-33)));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_tag(&writer, mpack_tag_int(-128)));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_tag(&writer, mpack_tag_int(-129)));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_tag(&writer, mpack_tag_int(-32768)));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_tag(&writer, mpack_tag_int(-32769)));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(INT64_C(-2147483648))));

    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(INT64_C(-2147483649))));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(INT64_MIN)));

}

static void test_write_simple_misc() {
    char buf[4096];

    TEST_SIMPLE_WRITE("\xc0", mpack_write_nil(&writer));
    TEST_SIMPLE_WRITE("\xc2", mpack_write_bool(&writer, false));
    TEST_SIMPLE_WRITE("\xc3", mpack_write_bool(&writer, true));
    TEST_SIMPLE_WRITE("\xc2", mpack_write_false(&writer));
    TEST_SIMPLE_WRITE("\xc3", mpack_write_true(&writer));

    // we just test a few floats for now. this could certainly be extended to
    // test more values like subnormal floats, infinities, etc.
    TEST_SIMPLE_WRITE("\xca\x00\x00\x00\x00", mpack_write_float(&writer, 0.0f));
    TEST_SIMPLE_WRITE("\xca\x40\x2d\xf3\xb6", mpack_write_float(&writer, 2.718f));
    TEST_SIMPLE_WRITE("\xca\xc0\x2d\xf3\xb6", mpack_write_float(&writer, -2.718f));
    TEST_SIMPLE_WRITE("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", mpack_write_double(&writer, 0.0));
    TEST_SIMPLE_WRITE("\xcb\x40\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write_double(&writer, 3.14159265));
    TEST_SIMPLE_WRITE("\xcb\xc0\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write_double(&writer, -3.14159265));

}

#ifdef MPACK_MALLOC
static void test_write_basic_structures() {
    char* buf;
    size_t size;
    mpack_writer_t writer;

    // []
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 0);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH("\x90");

    // [nil]
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 1);
    mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH("\x91\xc0");

    // range(15)
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 15);
        for (int i = 0; i < 15; ++i)
            mpack_write_int(&writer, i);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(
        "\x9f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
        );

    // range(16) (larger than infix)
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 16);
        for (int i = 0; i < 16; ++i)
            mpack_write_int(&writer, i);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(
        "\xdc\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
        "\x0d\x0e\x0f"
        );

    // UINT16_MAX nils
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, UINT16_MAX);
        for (int i = 0; i < UINT16_MAX; ++i)
            mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    {
        const char prefix[] = "\xdc\xff\xff";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, buf, sizeof(prefix)-1) == 0, "array prefix is incorrect");
        TEST_TRUE(size == UINT16_MAX + sizeof(prefix)-1);
    }
    if (buf)
        MPACK_FREE(buf);

    // UINT16_MAX+1 nils (largest category)
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, UINT16_MAX+1);
        for (int i = 0; i < UINT16_MAX+1; ++i)
            mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    {
        const char prefix[] = "\xdd\x00\x01\x00\x00";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, buf, sizeof(prefix)-1) == 0, "array prefix is incorrect");
        TEST_TRUE(size == UINT16_MAX+1 + sizeof(prefix)-1);
    }
    if (buf)
        MPACK_FREE(buf);

    // {}
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 0);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH("\x80");

    // {nil:nil}
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 1);
        mpack_write_nil(&writer);
        mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH("\x81\xc0\xc0");

    // {0:0,1:1}
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 2);
        mpack_write_int(&writer, 0);
        mpack_write_int(&writer, 0);
        mpack_write_int(&writer, 1);
        mpack_write_int(&writer, 1);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH("\x82\x00\x00\x01\x01");

    // {0:1, 2:3, ..., 28:29}
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 15);
        for (int i = 0; i < 30; ++i)
            mpack_write_int(&writer, i);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(
        "\x8f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
        "\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d"
        );

    // {0:1, 2:3, ..., 28:29, 30:31} (larger than infix)
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 16);
        for (int i = 0; i < 32; ++i)
            mpack_write_int(&writer, i);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(
        "\xde\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
        "\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
        "\x1d\x1e\x1f"
        );

    // UINT16_MAX nil:nils
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, UINT16_MAX);
        for (int i = 0; i < UINT16_MAX*2; ++i)
            mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    {
        const char prefix[] = "\xde\xff\xff";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, buf, sizeof(prefix)-1) == 0, "map prefix is incorrect");
        TEST_TRUE(size == UINT16_MAX*2 + sizeof(prefix)-1);
    }
    if (buf)
        MPACK_FREE(buf);

    // UINT16_MAX+1 nil:nils (largest category)
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, UINT16_MAX+1);
        for (int i = 0; i < (UINT16_MAX+1)*2; ++i)
            mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    {
        const char prefix[] = "\xdf\x00\x01\x00\x00";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, buf, sizeof(prefix)-1) == 0, "map prefix is incorrect");
        TEST_TRUE(size == (UINT16_MAX+1)*2 + sizeof(prefix)-1);
    }
    if (buf)
        MPACK_FREE(buf);
}

static void test_write_small_structure_trees() {
    char* buf;
    size_t size;
    mpack_writer_t writer;

    // [[]]
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 1);
        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH("\x91\x90");

    // [[], [0], [1, 2]]
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 3);
        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);
        mpack_start_array(&writer, 1);
            mpack_write_int(&writer, 0);
        mpack_finish_array(&writer);
        mpack_start_array(&writer, 2);
            mpack_write_int(&writer, 1);
            mpack_write_int(&writer, 2);
        mpack_finish_array(&writer);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH("\x93\x90\x91\x00\x92\x01\x02");

    // miscellaneous tree of arrays of various small sizes
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_array(&writer, 5);

        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 1);
            mpack_write_nil(&writer);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 2);
            mpack_start_array(&writer, 0);
            mpack_finish_array(&writer);
            mpack_start_array(&writer, 1);
                mpack_write_nil(&writer);
            mpack_finish_array(&writer);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 15);
            for (int i = 0; i < 15; ++i)
                mpack_write_int(&writer, i);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 16);
            for (int i = 0; i < 16; ++i)
                mpack_write_int(&writer, i);
        mpack_finish_array(&writer);

    mpack_finish_array(&writer);

    TEST_DESTROY_MATCH(
        "\x95\x90\x91\xc0\x92\x90\x91\xc0\x9f\x00\x01\x02\x03\x04\x05\x06"
        "\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\xdc\x00\x10\x00\x01\x02\x03\x04"
        "\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
        );


    // miscellaneous tree of maps of various small sizes
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 5);

        mpack_write_int(&writer, 0);
        mpack_start_map(&writer, 0);
        mpack_finish_map(&writer);

        mpack_write_int(&writer, 1);
        mpack_start_map(&writer, 1);
            mpack_write_int(&writer, 0);
            mpack_write_nil(&writer);
        mpack_finish_map(&writer);

        mpack_write_int(&writer, 2);
        mpack_start_map(&writer, 2);
            mpack_write_int(&writer, 0);
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_write_int(&writer, 1);
            mpack_start_map(&writer, 1);
                mpack_write_nil(&writer);
                mpack_write_nil(&writer);
            mpack_finish_map(&writer);
        mpack_finish_map(&writer);

        mpack_write_int(&writer, 3);
        mpack_start_map(&writer, 15);
            for (int i = 0; i < 15; ++i) {
                mpack_write_int(&writer, i);
                mpack_write_int(&writer, i);
            }
        mpack_finish_map(&writer);

        mpack_write_int(&writer, 4);
        mpack_start_map(&writer, 16);
            for (int i = 0; i < 16; ++i) {
                mpack_write_int(&writer, i);
                mpack_write_int(&writer, i);
            }
        mpack_finish_map(&writer);

    mpack_finish_map(&writer);

    TEST_DESTROY_MATCH(
        "\x85\x00\x80\x01\x81\x00\xc0\x02\x82\x00\x80\x01\x81\xc0\xc0\x03"
        "\x8f\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06\x06\x07"
        "\x07\x08\x08\x09\x09\x0a\x0a\x0b\x0b\x0c\x0c\x0d\x0d\x0e\x0e\x04"
        "\xde\x00\x10\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06"
        "\x06\x07\x07\x08\x08\x09\x09\x0a\x0a\x0b\x0b\x0c\x0c\x0d\x0d\x0e"
        "\x0e\x0f\x0f"
        );


    // miscellaneous mix of maps and arrays of various small sizes
    mpack_writer_init_growable(&writer, &buf, &size);
    mpack_start_map(&writer, 5);

        mpack_write_int(&writer, -47);
        mpack_start_array(&writer, 1);
            mpack_write_nil(&writer);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);
        mpack_start_map(&writer, 1);
            mpack_write_nil(&writer);
            mpack_write_int(&writer, 0);
        mpack_finish_map(&writer);

        mpack_write_nil(&writer);
        mpack_start_map(&writer, 2);
            mpack_write_nil(&writer);
            mpack_start_array(&writer, 0);
            mpack_finish_array(&writer);
            mpack_write_int(&writer, 4);
            mpack_write_int(&writer, 5);
        mpack_finish_map(&writer);

        mpack_write_cstr(&writer, "hello");
        mpack_start_array(&writer, 3);
            mpack_write_cstr(&writer, "bonjour");
            mpack_write_nil(&writer);
            mpack_write_int(&writer, -1);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 1);
            mpack_write_int(&writer, 92);
        mpack_finish_array(&writer);
        mpack_write_int(&writer, 350);

    mpack_finish_map(&writer);

    TEST_DESTROY_MATCH(
        "\x85\xd0\xd1\x91\xc0\x90\x81\xc0\x00\xc0\x82\xc0\x90\x04\x05\xa5"
        "\x68\x65\x6c\x6c\x6f\x93\xa7\x62\x6f\x6e\x6a\x6f\x75\x72\xc0\xff"
        "\x91\x5c\xcd\x01\x5e"
        );

}

static bool test_write_deep_growth() {

    // test a growable writer with a very deep stack and lots
    // of data to see if both the growable buffer and the tracking
    // stack grow properly. we allow mpack_error_memory as an
    // error (since it will be simulated by the failure system.)

    char* buf;
    size_t size;
    mpack_writer_t writer;

    #define TEST_POSSIBLE_FAILURE() do { \
        if (mpack_writer_error(&writer) == mpack_error_memory) { \
            TEST_TRUE(test_write_error == mpack_error_memory, "writer error handler was not called?"); \
            test_write_error = mpack_ok; \
            mpack_writer_destroy(&writer); \
            TEST_TRUE(buf == NULL); \
            return false; \
        } \
    } while (0)

    mpack_writer_init_growable(&writer, &buf, &size);
    if (mpack_writer_error(&writer) == mpack_error_memory) {
        mpack_writer_destroy(&writer);
        TEST_TRUE(buf == NULL);
        return false;
    }

    TEST_TRUE(test_write_error == mpack_ok);
    mpack_writer_set_error_handler(&writer, test_write_error_handler);

    const int depth = 40;
    const int nums = 1000;

    for (int i = 0; i < depth; ++i) {
        mpack_start_array(&writer, 1);
        TEST_POSSIBLE_FAILURE();
    }

    mpack_start_array(&writer, nums);
    TEST_POSSIBLE_FAILURE();
    for (int i = 0; i < nums; ++i) {
        mpack_write_u64(&writer, UINT64_MAX);
        TEST_POSSIBLE_FAILURE();
    }
    mpack_finish_array(&writer);
    TEST_POSSIBLE_FAILURE();

    for (int i = 0; i < depth; ++i) {
        mpack_finish_array(&writer);
        TEST_POSSIBLE_FAILURE();
    }

    #undef TEST_POSSIBLE_FAILURE

    mpack_error_t error = mpack_writer_destroy(&writer);
    if (error == mpack_ok) {
        MPACK_FREE(buf);
        return true;
    }
    if (error == mpack_error_memory) {
        TEST_TRUE(buf == NULL);
        return false;
    }
    TEST_TRUE(false, "unexpected error state %i (%s)", (int)error, mpack_error_to_string(error));
    return true;

}
#endif

#if MPACK_WRITE_TRACKING
static void test_write_tracking() {
    char buf[4096];
    mpack_writer_t writer;

    // cancel
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_map(&writer, 5);
    mpack_start_array(&writer, 5);
    mpack_writer_destroy_cancel(&writer);

    // finishing type when nothing was open
    mpack_writer_init(&writer, buf, sizeof(buf));
    TEST_BREAK((mpack_finish_map(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // closing unfinished type
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 1);
    TEST_BREAK((mpack_finish_array(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // writing elements in a string
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_str(&writer, 50);
    TEST_BREAK((mpack_write_nil(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // writing too many elements
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 0);
    TEST_BREAK((mpack_write_nil(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // writing bytes with nothing open
    mpack_writer_init(&writer, buf, sizeof(buf));
    TEST_BREAK((mpack_write_bytes(&writer, "test", 4), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // writing bytes in an array
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 50);
    TEST_BREAK((mpack_write_bytes(&writer, "test", 4), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // writing too many bytes
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_str(&writer, 2);
    TEST_BREAK((mpack_write_bytes(&writer, "test", 4), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

}
#endif

void test_writes() {
    /*
    const char c[] =
        "\x85\xd0\xd1\x91\xc0\x90\x81\xc0\x00\xc0\x82\xc0\x90\x04\x05\xa5"
        "\x68\x65\x6c\x6c\x6f\x93\xa7\x62\x6f\x6e\x6a\x6f\x75\x72\xc0\xff"
        "\x91\x5c\xcd\x01\x5e"
        ;
    mpack_debug_print(c, sizeof(c)-1);
    */

    test_write_simple_auto_int();
    test_write_simple_size_int_fixnums();
    test_write_simple_size_int();
    test_write_simple_tag_int();
    test_write_simple_misc();

    #ifdef MPACK_MALLOC
    test_write_basic_structures();
    test_write_small_structure_trees();
    test_system_fail_until_ok(&test_write_deep_growth);
    #endif

    #if MPACK_WRITE_TRACKING
    test_write_tracking();
    #endif
}

#endif

