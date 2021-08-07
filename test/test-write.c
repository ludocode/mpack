/*
 * Copyright (c) 2015-2021 Nicholas Fraser and the MPack authors
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

static char buf[4096];

static const char* quick_brown_fox = "The quick brown fox jumps over a lazy dog.";

mpack_error_t test_write_error = mpack_ok;

void test_write_error_handler(mpack_writer_t* writer, mpack_error_t error) {
    TEST_TRUE(test_write_error == mpack_ok, "error handler was called multiple times");
    TEST_TRUE(error != mpack_ok, "error handler was called with mpack_ok");
    TEST_TRUE(mpack_writer_error(writer) == error, "writer error does not match given error");
    test_write_error = error;
}

// writes ints using the auto int()/uint() functions
static void test_write_simple_auto_int(void) {
    mpack_writer_t writer;

    // positive fixnums
    TEST_SIMPLE_WRITE("\x00", mpack_write_uint(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_uint(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_uint(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_uint(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_uint(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7e", mpack_write_uint(&writer, 0x7e));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_uint(&writer, 0x7f));

    // positive fixnums with signed int functions
    TEST_SIMPLE_WRITE("\x00", mpack_write_int(&writer, 0));
    TEST_SIMPLE_WRITE("\x01", mpack_write_int(&writer, 1));
    TEST_SIMPLE_WRITE("\x02", mpack_write_int(&writer, 2));
    TEST_SIMPLE_WRITE("\x0f", mpack_write_int(&writer, 0x0f));
    TEST_SIMPLE_WRITE("\x10", mpack_write_int(&writer, 0x10));
    TEST_SIMPLE_WRITE("\x7e", mpack_write_int(&writer, 0x7e));
    TEST_SIMPLE_WRITE("\x7f", mpack_write_int(&writer, 0x7f));

    // negative fixnums
    TEST_SIMPLE_WRITE("\xff", mpack_write_int(&writer, -1));
    TEST_SIMPLE_WRITE("\xfe", mpack_write_int(&writer, -2));
    TEST_SIMPLE_WRITE("\xf0", mpack_write_int(&writer, -16));
    TEST_SIMPLE_WRITE("\xe1", mpack_write_int(&writer, -31));
    TEST_SIMPLE_WRITE("\xe0", mpack_write_int(&writer, -32));

    // uints
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_uint(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_uint(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_uint(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_uint(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_uint(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_uint(&writer, 0xffffffff));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_uint(&writer, MPACK_UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_uint(&writer, MPACK_UINT64_C(0xffffffffffffffff)));

    // positive ints with signed value
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_int(&writer, 0x80));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_int(&writer, 0xff));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_int(&writer, 0x100));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_int(&writer, 0xffff));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_int(&writer, 0x10000));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_int(&writer, MPACK_INT64_C(0xffffffff)));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_int(&writer, MPACK_INT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_int(&writer, MPACK_INT64_C(0x7fffffffffffffff)));

    // ints
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_int(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_int(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_int(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_int(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_int(&writer, -32769));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_int(&writer, MPACK_INT64_C(-2147483648)));

    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_int(&writer, MPACK_INT64_C(-2147483649)));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_int(&writer, MPACK_INT64_MIN));

}

// writes ints using the sized iXX()/uXX() functions
static void test_write_simple_size_int_fixnums(void) {
    mpack_writer_t writer;

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

static void test_write_simple_size_int(void) {
    mpack_writer_t writer;

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
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_u64(&writer, MPACK_UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_u64(&writer, MPACK_UINT64_C(0xffffffffffffffff)));

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
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_i64(&writer, MPACK_INT64_C(0xffffffff)));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_i64(&writer, MPACK_UINT64_C(0x100000000)));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_i64(&writer, MPACK_UINT64_C(0x7fffffffffffffff)));

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
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_i32(&writer, MPACK_INT64_C(-2147483648)));
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_i64(&writer, MPACK_INT64_C(-2147483648)));

    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_i64(&writer, -33));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_i64(&writer, -128));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_i64(&writer, -129));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_i64(&writer, -32768));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_i64(&writer, -32769));
    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_i64(&writer, MPACK_INT64_C(-2147483649)));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_i64(&writer, MPACK_INT64_MIN));

}

// writes ints using the dynamic tag writer function
static void test_write_simple_tag_int(void) {
    mpack_writer_t writer;

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
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_uint(MPACK_UINT64_C(0x100000000))));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_uint(MPACK_UINT64_C(0xffffffffffffffff))));

    // positive ints with signed value
    TEST_SIMPLE_WRITE("\xcc\x80", mpack_write_tag(&writer, mpack_tag_int(0x80)));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write_tag(&writer, mpack_tag_int(0xff)));
    TEST_SIMPLE_WRITE("\xcd\x01\x00", mpack_write_tag(&writer, mpack_tag_int(0x100)));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write_tag(&writer, mpack_tag_int(0xffff)));
    TEST_SIMPLE_WRITE("\xce\x00\x01\x00\x00", mpack_write_tag(&writer, mpack_tag_int(0x10000)));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_C(0xffffffff))));
    TEST_SIMPLE_WRITE("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_C(0x100000000))));
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_C(0x7fffffffffffffff))));

    // ints
    TEST_SIMPLE_WRITE("\xd0\xdf", mpack_write_tag(&writer, mpack_tag_int(-33)));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write_tag(&writer, mpack_tag_int(-128)));
    TEST_SIMPLE_WRITE("\xd1\xff\x7f", mpack_write_tag(&writer, mpack_tag_int(-129)));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write_tag(&writer, mpack_tag_int(-32768)));
    TEST_SIMPLE_WRITE("\xd2\xff\xff\x7f\xff", mpack_write_tag(&writer, mpack_tag_int(-32769)));

    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_C(-2147483648))));

    TEST_SIMPLE_WRITE("\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_C(-2147483649))));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write_tag(&writer, mpack_tag_int(MPACK_INT64_MIN)));

}

static void test_write_simple_misc(void) {
    mpack_writer_t writer;

    TEST_SIMPLE_WRITE("\xc0", mpack_write_nil(&writer));
    TEST_SIMPLE_WRITE("\xc2", mpack_write_bool(&writer, false));
    TEST_SIMPLE_WRITE("\xc3", mpack_write_bool(&writer, true));
    TEST_SIMPLE_WRITE("\xc2", mpack_write_false(&writer));
    TEST_SIMPLE_WRITE("\xc3", mpack_write_true(&writer));

    // we just test a few floats for now. this could certainly be extended to
    // test more values like subnormal floats, infinities, etc.
    #if MPACK_FLOAT
    TEST_SIMPLE_WRITE("\xca\x00\x00\x00\x00", mpack_write_float(&writer, 0.0f));
    TEST_SIMPLE_WRITE("\xca\x40\x2d\xf3\xb6", mpack_write_float(&writer, 2.718f));
    TEST_SIMPLE_WRITE("\xca\xc0\x2d\xf3\xb6", mpack_write_float(&writer, -2.718f));
    #endif
    #if MPACK_DOUBLE
    TEST_SIMPLE_WRITE("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", mpack_write_double(&writer, 0.0));
    TEST_SIMPLE_WRITE("\xcb\x40\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write_double(&writer, 3.14159265));
    TEST_SIMPLE_WRITE("\xcb\xc0\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write_double(&writer, -3.14159265));
    #endif

    TEST_SIMPLE_WRITE("\xde\xad\xbe\xef", mpack_write_object_bytes(&writer, "\xde\xad\xbe\xef", 4));

    #ifdef MPACK_MALLOC
    // test writing nothing
    char* growable_buf;
    size_t size;
    mpack_writer_init_growable(&writer, &growable_buf, &size);
    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(size == 0);
    TEST_TRUE(growable_buf != NULL);
    MPACK_FREE(growable_buf);

    // test growing by many steps at once (the initial buffer size during tests
    // is 32, and the lipsum string is >700 characters)
    mpack_writer_init_growable(&writer, &growable_buf, &size);
    mpack_write_cstr(&writer, lipsum);
    TEST_WRITER_DESTROY_NOERROR(&writer);
    TEST_TRUE(size == MPACK_TAG_SIZE_STR16 + strlen(lipsum));
    TEST_TRUE(growable_buf[0] == '\xda');
    TEST_TRUE(mpack_load_u16(growable_buf + 1) == strlen(lipsum));
    TEST_TRUE(memcmp(growable_buf + MPACK_TAG_SIZE_STR16, lipsum, strlen(lipsum)) == 0);
    MPACK_FREE(growable_buf);
    #endif
}

#if MPACK_EXTENSIONS
static void test_write_timestamp(void) {
    mpack_writer_t writer;

    TEST_SIMPLE_WRITE("\xd6\xff\x00\x00\x00\x00", mpack_write_timestamp_seconds(&writer, 0));
    TEST_SIMPLE_WRITE("\xd6\xff\x00\x00\x01\x00", mpack_write_timestamp(&writer, 256, 0));
    TEST_SIMPLE_WRITE("\xd6\xff\xfe\xdc\xba\x98", mpack_write_timestamp_seconds(&writer, 4275878552u));
    TEST_SIMPLE_WRITE("\xd6\xff\xff\xff\xff\xff", mpack_write_timestamp(&writer, MPACK_UINT32_MAX, 0));

    TEST_SIMPLE_WRITE("\xd7\xff\x00\x00\x00\x03\x00\x00\x00\x00",
            mpack_write_timestamp_seconds(&writer, MPACK_INT64_C(12884901888)));
    TEST_SIMPLE_WRITE("\xd7\xff\xee\x6b\x27\xfc\x00\x00\x00\x00",
            mpack_write_timestamp(&writer, 0, MPACK_TIMESTAMP_NANOSECONDS_MAX));
    TEST_SIMPLE_WRITE("\xd7\xff\xee\x6b\x27\xff\xff\xff\xff\xff",
            mpack_write_timestamp(&writer, MPACK_INT64_C(17179869183), MPACK_TIMESTAMP_NANOSECONDS_MAX));

    TEST_SIMPLE_WRITE("\xc7\x0c\xff\x00\x00\x00\x01\xff\xff\xff\xff\xff\xff\xff\xff",
            mpack_write_timestamp(&writer, -1, 1));
    mpack_timestamp_t timestamp = {MPACK_INT64_MAX, MPACK_TIMESTAMP_NANOSECONDS_MAX};
    TEST_SIMPLE_WRITE("\xc7\x0c\xff\x3b\x9a\xc9\xff\x7f\xff\xff\xff\xff\xff\xff\xff",
            mpack_write_timestamp_struct(&writer, timestamp));
    TEST_SIMPLE_WRITE("\xc7\x0c\xff\x3b\x9a\xc9\xff\x80\x00\x00\x00\x00\x00\x00\x00",
            mpack_write_timestamp(&writer, MPACK_INT64_MIN, MPACK_TIMESTAMP_NANOSECONDS_MAX));

    mpack_writer_init(&writer, buf, sizeof(buf));
    TEST_BREAK((mpack_write_timestamp(&writer, 0, 1000000000), true));
    TEST_BREAK((mpack_write_timestamp(&writer, 0, MPACK_UINT32_MAX), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);
}
#endif

#ifdef MPACK_MALLOC
static void test_write_tag_tracking(void) {
    char* out;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &out, &size);

    mpack_start_array(&writer, 8);
        mpack_write_tag(&writer, mpack_tag_nil());
        mpack_write_tag(&writer, mpack_tag_bool(true));
        mpack_write_tag(&writer, mpack_tag_bool(false));
        mpack_write_tag(&writer, mpack_tag_uint(4));
        mpack_write_tag(&writer, mpack_tag_int(-3));
        mpack_write_tag(&writer, mpack_tag_str(0));
        mpack_finish_str(&writer);
        mpack_write_tag(&writer, mpack_tag_bin(0));
        mpack_finish_bin(&writer);
        mpack_write_tag(&writer, mpack_tag_array(1));
            mpack_write_tag(&writer, mpack_tag_nil());
        mpack_finish_array(&writer);
    mpack_finish_array(&writer);

    TEST_DESTROY_MATCH(out, "\x98\xC0\xC3\xC2\x04\xFD\xA0\xC4\x00\x91\xC0");
}

static void test_write_basic_structures(void) {
    char* out;
    size_t size;
    mpack_writer_t writer;
    int i;

    // we use a mix of int writers below to test their tracking.

    // []
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, 0);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(out, "\x90");

    // [nil]
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, 1);
    mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(out, "\x91\xc0");

    // range(15)
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, 15);
        for (i = 0; i < 15; ++i)
            mpack_write_i32(&writer, i);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(out,
        "\x9f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
        );

    // range(16) (larger than infix)
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, 16);
        for (i = 0; i < 16; ++i)
            mpack_write_u32(&writer, (uint32_t)i);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(out,
        "\xdc\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
        "\x0d\x0e\x0f"
        );

    // MPACK_UINT16_MAX nils
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, MPACK_UINT16_MAX);
        for (i = 0; i < MPACK_UINT16_MAX; ++i)
            mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    {
        const char prefix[] = "\xdc\xff\xff";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, out, sizeof(prefix)-1) == 0, "array prefix is incorrect");
        TEST_TRUE(size == MPACK_UINT16_MAX + sizeof(prefix)-1);
    }
    if (out)
        MPACK_FREE(out);

    // MPACK_UINT16_MAX+1 nils (largest category)
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, MPACK_UINT16_MAX+1);
        for (i = 0; i < MPACK_UINT16_MAX+1; ++i)
            mpack_write_nil(&writer);
    mpack_finish_array(&writer);
    {
        const char prefix[] = "\xdd\x00\x01\x00\x00";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, out, sizeof(prefix)-1) == 0, "array prefix is incorrect");
        TEST_TRUE(size == MPACK_UINT16_MAX+1 + sizeof(prefix)-1);
    }
    if (out)
        MPACK_FREE(out);

    // {}
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, 0);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(out, "\x80");

    // {nil:nil}
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, 1);
        mpack_write_nil(&writer);
        mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(out, "\x81\xc0\xc0");

    // {0:0,1:1}
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, 2);
        mpack_write_i8(&writer, 0);
        mpack_write_i16(&writer, 0);
        mpack_write_u8(&writer, 1);
        mpack_write_u16(&writer, 1);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(out, "\x82\x00\x00\x01\x01");

    // {0:1, 2:3, ..., 28:29}
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, 15);
        for (i = 0; i < 30; ++i)
            mpack_write_i8(&writer, (int8_t)i);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(out,
        "\x8f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
        "\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d"
        );

    // {0:1, 2:3, ..., 28:29, 30:31} (larger than infix)
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, 16);
        for (i = 0; i < 32; ++i)
            mpack_write_int(&writer, i);
    mpack_finish_map(&writer);
    TEST_DESTROY_MATCH(out,
        "\xde\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
        "\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
        "\x1d\x1e\x1f"
        );

    // MPACK_UINT16_MAX nil:nils
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, MPACK_UINT16_MAX);
        for (i = 0; i < MPACK_UINT16_MAX*2; ++i)
            mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    {
        const char prefix[] = "\xde\xff\xff";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, out, sizeof(prefix)-1) == 0, "map prefix is incorrect");
        TEST_TRUE(size == MPACK_UINT16_MAX*2 + sizeof(prefix)-1);
    }
    if (out)
        MPACK_FREE(out);

    // MPACK_UINT16_MAX+1 nil:nils (largest category)
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_map(&writer, MPACK_UINT16_MAX+1);
        for (i = 0; i < (MPACK_UINT16_MAX+1)*2; ++i)
            mpack_write_nil(&writer);
    mpack_finish_map(&writer);
    {
        const char prefix[] = "\xdf\x00\x01\x00\x00";
        TEST_WRITER_DESTROY_NOERROR(&writer);
        TEST_TRUE(memcmp(prefix, out, sizeof(prefix)-1) == 0, "map prefix is incorrect");
        TEST_TRUE(size == (MPACK_UINT16_MAX+1)*2 + sizeof(prefix)-1);
    }
    if (out)
        MPACK_FREE(out);
}

static void test_write_small_structure_trees(void) {
    char* out;
    size_t size;
    mpack_writer_t writer;
    int i;

    // [[]]
    mpack_writer_init_growable(&writer, &out, &size);
    mpack_start_array(&writer, 1);
        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);
    mpack_finish_array(&writer);
    TEST_DESTROY_MATCH(out, "\x91\x90");

    // [[], [0], [1, 2]]
    mpack_writer_init_growable(&writer, &out, &size);
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
    TEST_DESTROY_MATCH(out, "\x93\x90\x91\x00\x92\x01\x02");

    // miscellaneous tree of arrays of various small sizes
    mpack_writer_init_growable(&writer, &out, &size);
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
            for (i = 0; i < 15; ++i)
                mpack_write_int(&writer, i);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 16);
            for (i = 0; i < 16; ++i)
                mpack_write_int(&writer, i);
        mpack_finish_array(&writer);

    mpack_finish_array(&writer);

    TEST_DESTROY_MATCH(out,
        "\x95\x90\x91\xc0\x92\x90\x91\xc0\x9f\x00\x01\x02\x03\x04\x05\x06"
        "\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\xdc\x00\x10\x00\x01\x02\x03\x04"
        "\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
        );


    // miscellaneous tree of maps of various small sizes
    mpack_writer_init_growable(&writer, &out, &size);
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
            for (i = 0; i < 15; ++i) {
                mpack_write_int(&writer, i);
                mpack_write_int(&writer, i);
            }
        mpack_finish_map(&writer);

        mpack_write_int(&writer, 4);
        mpack_start_map(&writer, 16);
            for (i = 0; i < 16; ++i) {
                mpack_write_int(&writer, i);
                mpack_write_int(&writer, i);
            }
        mpack_finish_map(&writer);

    mpack_finish_map(&writer);

    TEST_DESTROY_MATCH(out,
        "\x85\x00\x80\x01\x81\x00\xc0\x02\x82\x00\x80\x01\x81\xc0\xc0\x03"
        "\x8f\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06\x06\x07"
        "\x07\x08\x08\x09\x09\x0a\x0a\x0b\x0b\x0c\x0c\x0d\x0d\x0e\x0e\x04"
        "\xde\x00\x10\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06"
        "\x06\x07\x07\x08\x08\x09\x09\x0a\x0a\x0b\x0b\x0c\x0c\x0d\x0d\x0e"
        "\x0e\x0f\x0f"
        );


    // miscellaneous mix of maps and arrays of various small sizes
    mpack_writer_init_growable(&writer, &out, &size);
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
            mpack_write_cstr_or_nil(&writer, "bonjour");
            mpack_write_cstr_or_nil(&writer, NULL);
            mpack_write_int(&writer, -1);
        mpack_finish_array(&writer);

        mpack_start_array(&writer, 1);
            mpack_write_int(&writer, 92);
        mpack_finish_array(&writer);
        mpack_write_int(&writer, 350);

    mpack_finish_map(&writer);

    TEST_DESTROY_MATCH(out,
        "\x85\xd0\xd1\x91\xc0\x90\x81\xc0\x00\xc0\x82\xc0\x90\x04\x05\xa5"
        "\x68\x65\x6c\x6c\x6f\x93\xa7\x62\x6f\x6e\x6a\x6f\x75\x72\xc0\xff"
        "\x91\x5c\xcd\x01\x5e"
        );

}

static bool test_write_deep_growth(void) {

    // test a growable writer with a very deep stack and lots
    // of data to see if both the growable buffer and the tracking
    // stack grow properly. we allow mpack_error_memory as an
    // error (since it will be simulated by the failure system.)

    char* out;
    size_t size;
    mpack_writer_t writer;

    #define TEST_POSSIBLE_FAILURE() do { \
        if (mpack_writer_error(&writer) == mpack_error_memory) { \
            TEST_TRUE(test_write_error == mpack_error_memory, "writer error handler was not called?"); \
            test_write_error = mpack_ok; \
            mpack_writer_destroy(&writer); \
            TEST_TRUE(out == NULL); \
            return false; \
        } \
    } while (0)

    mpack_writer_init_growable(&writer, &out, &size);
    if (mpack_writer_error(&writer) == mpack_error_memory) {
        mpack_writer_destroy(&writer);
        TEST_TRUE(out == NULL);
        return false;
    }

    TEST_TRUE(test_write_error == mpack_ok);
    mpack_writer_set_error_handler(&writer, test_write_error_handler);

    const int depth = 40;
    const int nums = 1000;

    int i;
    for (i = 0; i < depth; ++i) {
        mpack_start_array(&writer, 1);
        TEST_POSSIBLE_FAILURE();
    }

    mpack_start_array(&writer, (uint32_t)nums);
    TEST_POSSIBLE_FAILURE();
    for (i = 0; i < nums; ++i) {
        mpack_write_u64(&writer, MPACK_UINT64_MAX);
        TEST_POSSIBLE_FAILURE();
    }
    mpack_finish_array(&writer);
    TEST_POSSIBLE_FAILURE();

    for (i = 0; i < depth; ++i) {
        mpack_finish_array(&writer);
        TEST_POSSIBLE_FAILURE();
    }

    #undef TEST_POSSIBLE_FAILURE

    mpack_error_t error = mpack_writer_destroy(&writer);
    if (error == mpack_ok) {
        MPACK_FREE(out);
        return true;
    }
    if (error == mpack_error_memory) {
        TEST_TRUE(out == NULL);
        return false;
    }
    TEST_TRUE(false, "unexpected error state %i (%s)", (int)error, mpack_error_to_string(error));
    return true;

}
#endif

#if MPACK_WRITE_TRACKING
static void test_write_tracking(void) {
    mpack_writer_t writer;

    // cancel
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_map(&writer, 5);
    mpack_start_array(&writer, 5);
    mpack_writer_flag_error(&writer, mpack_error_data);
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_data);

    // finishing type when nothing was open
    mpack_writer_init(&writer, buf, sizeof(buf));
    TEST_BREAK((mpack_finish_map(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // closing unfinished type
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 1);
    TEST_BREAK((mpack_finish_array(&writer), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);

    // closing wrong type
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_start_array(&writer, 0);
    TEST_BREAK((mpack_finish_map(&writer), true));
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

#if MPACK_HAS_GENERIC
static void test_write_generic(void) {
    mpack_writer_t writer;

    // int8
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (int8_t)MPACK_INT8_MAX));
    TEST_SIMPLE_WRITE("\x01", mpack_write(&writer, (int8_t)1));
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (int8_t)0));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write(&writer, (int8_t)MPACK_INT8_MIN));

    // int16
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write(&writer, (int16_t)MPACK_INT16_MAX));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (int16_t)MPACK_INT8_MAX));
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (int16_t)0));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write(&writer, (int16_t)MPACK_INT8_MIN));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write(&writer, (int16_t)MPACK_INT16_MIN));

    // int32
    TEST_SIMPLE_WRITE("\xce\x7f\xff\xff\xff", mpack_write(&writer, (int32_t)MPACK_INT32_MAX));
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write(&writer, (int32_t)MPACK_INT16_MAX));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (int32_t)MPACK_INT8_MAX));
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (int32_t)0));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write(&writer, (int32_t)MPACK_INT8_MIN));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write(&writer, (int32_t)MPACK_INT16_MIN));
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write(&writer, (int32_t)MPACK_INT32_MIN));

    // int64
    TEST_SIMPLE_WRITE("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write(&writer, (int64_t)MPACK_INT64_MAX));
    TEST_SIMPLE_WRITE("\xce\x7f\xff\xff\xff", mpack_write(&writer, (int64_t)MPACK_INT32_MAX));
    TEST_SIMPLE_WRITE("\xcd\x7f\xff", mpack_write(&writer, (int64_t)MPACK_INT16_MAX));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (int64_t)MPACK_INT8_MAX));
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (int64_t)0));
    TEST_SIMPLE_WRITE("\xd0\x80", mpack_write(&writer, (int64_t)MPACK_INT8_MIN));
    TEST_SIMPLE_WRITE("\xd1\x80\x00", mpack_write(&writer, (int64_t)MPACK_INT16_MIN));
    TEST_SIMPLE_WRITE("\xd2\x80\x00\x00\x00", mpack_write(&writer, (int64_t)MPACK_INT32_MIN));
    TEST_SIMPLE_WRITE("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", mpack_write(&writer, (int64_t)MPACK_INT64_MIN));

    // uint8
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (uint8_t)0));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (uint8_t)127));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write(&writer, (uint8_t)MPACK_UINT8_MAX));

    // uint16
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (uint16_t)0));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (uint16_t)127));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write(&writer, (uint16_t)MPACK_UINT8_MAX));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write(&writer, (uint16_t)MPACK_UINT16_MAX));

    // uint32
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (uint32_t)0));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (uint32_t)127));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write(&writer, (uint32_t)MPACK_UINT8_MAX));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write(&writer, (uint32_t)MPACK_UINT16_MAX));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write(&writer, (uint32_t)MPACK_UINT32_MAX));

    // uint64
    TEST_SIMPLE_WRITE("\x00", mpack_write(&writer, (uint64_t)0));
    TEST_SIMPLE_WRITE("\x7f", mpack_write(&writer, (uint64_t)127));
    TEST_SIMPLE_WRITE("\xcc\xff", mpack_write(&writer, (uint64_t)MPACK_UINT8_MAX));
    TEST_SIMPLE_WRITE("\xcd\xff\xff", mpack_write(&writer, (uint64_t)MPACK_UINT16_MAX));
    TEST_SIMPLE_WRITE("\xce\xff\xff\xff\xff", mpack_write(&writer, (uint64_t)MPACK_UINT32_MAX));
    TEST_SIMPLE_WRITE("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write(&writer, (uint64_t)MPACK_UINT64_MAX));

    // float and double
    // TODO: we just test a few floats for now. this could certainly be extended to
    // test more values like subnormal floats, infinities, etc.
    #if MPACK_FLOAT
    TEST_SIMPLE_WRITE("\xca\x00\x00\x00\x00", mpack_write(&writer, (float)0.0f));
    TEST_SIMPLE_WRITE("\xca\x40\x2d\xf3\xb6", mpack_write(&writer, (float)2.718f));
    TEST_SIMPLE_WRITE("\xca\xc0\x2d\xf3\xb6", mpack_write(&writer, (float)-2.718f));
    #endif
    #if MPACK_DOUBLE
    TEST_SIMPLE_WRITE("\xcb\x00\x00\x00\x00\x00\x00\x00\x00", mpack_write(&writer, (double)0.0));
    TEST_SIMPLE_WRITE("\xcb\x40\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write(&writer, (double)3.14159265));
    TEST_SIMPLE_WRITE("\xcb\xc0\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write(&writer, (double)-3.14159265));
    #endif

    // bool
    // TODO: when we pass direct true or false into the _Generic it seems not to emit the correct stream
    bool b = false;
    TEST_SIMPLE_WRITE("\xc2", mpack_write(&writer, b));
    b = true;
    TEST_SIMPLE_WRITE("\xc3", mpack_write(&writer, b));

    // char *
    TEST_SIMPLE_WRITE("\xc0", mpack_write(&writer, (char *)NULL));
    TEST_SIMPLE_WRITE("\xa4""1337", mpack_write(&writer, (char *)"1337"));

    // const char *
    TEST_SIMPLE_WRITE("\xc0", mpack_write(&writer, (const char *)NULL));
    TEST_SIMPLE_WRITE("\xa4""1337", mpack_write(&writer, (const char *)"1337"));

    // string literals
    TEST_SIMPLE_WRITE("\xa0", mpack_write(&writer, ""));
    TEST_SIMPLE_WRITE("\xa4""1337", mpack_write(&writer, "1337"));
}

static void test_write_generic_kv(void) {
    mpack_writer_t writer;
    char key[] = "foo";
    char value[] = "bar";

    // int8, int16, int32, int64
    TEST_SIMPLE_WRITE("\xa3""foo""\x7f", mpack_write_kv(&writer, key, (int8_t)MPACK_INT8_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xcd\x7f\xff", mpack_write_kv(&writer, key, (int16_t)MPACK_INT16_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xce\x7f\xff\xff\xff", mpack_write_kv(&writer, key, (int32_t)MPACK_INT32_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", mpack_write_kv(&writer, key, (int64_t)MPACK_INT64_MAX));

    // uint8, uint16, uint32, uint64
    TEST_SIMPLE_WRITE("\xa3""foo""\xcc\xff", mpack_write_kv(&writer, key, (uint8_t)MPACK_UINT8_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xcd\xff\xff", mpack_write_kv(&writer, key, (uint16_t)MPACK_UINT16_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xce\xff\xff\xff\xff", mpack_write_kv(&writer, key, (uint64_t)MPACK_UINT32_MAX));
    TEST_SIMPLE_WRITE("\xa3""foo""\xcf\xff\xff\xff\xff\xff\xff\xff\xff", mpack_write_kv(&writer, key, (uint64_t)MPACK_UINT64_MAX));

    // float, double and bool
    #if MPACK_FLOAT
    TEST_SIMPLE_WRITE("\xa3""foo""\xca\xc0\x2d\xf3\xb6", mpack_write_kv(&writer, key, (float)-2.718f));
    #endif
    #if MPACK_DOUBLE
    TEST_SIMPLE_WRITE("\xa3""foo""\xcb\xc0\x09\x21\xfb\x53\xc8\xd4\xf1", mpack_write_kv(&writer, key, (double)-3.14159265));
    #endif
    TEST_SIMPLE_WRITE("\xa3""foo""\xc2", mpack_write_kv(&writer, key, (bool)false));

    // char *, const char *, literal
    TEST_SIMPLE_WRITE("\xa3""foo""\xa3""bar", mpack_write_kv(&writer, key, (char *)value));
    TEST_SIMPLE_WRITE("\xa3""foo""\xa3""bar", mpack_write_kv(&writer, key, (const char *)value));
    TEST_SIMPLE_WRITE("\xa3""foo""\xa3""bar", mpack_write_kv(&writer, key, value));
    TEST_SIMPLE_WRITE("\xa3""foo""\xa3""bar", mpack_write_kv(&writer, key, "bar"));
}

#endif

static void test_write_utf8(void) {
    mpack_writer_t writer;

    // these test strings are mostly duplicated from test-expect.c, but
    // without the MessagePack header

    const char utf8_null[]            = "hello\x00world";
    const char utf8_valid[]           = " \xCF\x80 \xe4\xb8\xad \xf0\xa0\x80\xb6";
    const char utf8_trimmed[]         = "\xf0\xa0\x80\xb6";
    const char utf8_invalid[]         = " \x80 ";
    const char utf8_invalid_trimmed[] = "\xa0";
    const char utf8_truncated[]       = "\xf0\xa0";
    // we don't accept any of these UTF-8 variants; only pure UTF-8 is allowed.
    const char utf8_modified[]        = " \xc0\x80 ";
    const char utf8_cesu8[]           = " \xED\xA0\x81\xED\xB0\x80 ";
    const char utf8_wobbly[]          = " \xED\xA0\x81 ";

    // all non-UTF-8 writers should write these strings without error
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_null, (uint32_t)sizeof(utf8_null)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_valid, (uint32_t)sizeof(utf8_valid)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_trimmed, (uint32_t)sizeof(utf8_trimmed)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_invalid, (uint32_t)sizeof(utf8_invalid)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_invalid_trimmed, (uint32_t)sizeof(utf8_invalid_trimmed)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_truncated, (uint32_t)sizeof(utf8_truncated)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_modified, (uint32_t)sizeof(utf8_modified)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_cesu8, (uint32_t)sizeof(utf8_cesu8)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_str(&writer, utf8_wobbly, (uint32_t)sizeof(utf8_wobbly)-1));

    // as should the non-UTF-8 cstr writers
    // (the utf8_null test here is writing up to the null-terminator which
    // is not what is expected, but there is no way to test for that error.
    // null-terminated strings are bad!)
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_null));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_valid));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_trimmed));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_invalid));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_invalid_trimmed));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_truncated));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_modified));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_cesu8));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_cstr(&writer, utf8_wobbly));

    // test UTF-8 cstr writers
    // NUL is valid in UTF-8, so we allow it by the non-cstr API. (If you're
    // using it to write, you should also using some non-cstr API to
    // read, so the NUL will be safely handled.)
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8(&writer, utf8_null, (uint32_t)sizeof(utf8_null)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8(&writer, utf8_valid, (uint32_t)sizeof(utf8_valid)-1));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8(&writer, utf8_trimmed, (uint32_t)sizeof(utf8_trimmed)-1));
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_invalid, (uint32_t)sizeof(utf8_invalid)-1), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_invalid_trimmed, (uint32_t)sizeof(utf8_invalid_trimmed)-1), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_truncated, (uint32_t)sizeof(utf8_truncated)-1), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_modified, (uint32_t)sizeof(utf8_modified)-1), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_cesu8, (uint32_t)sizeof(utf8_cesu8)-1), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8(&writer, utf8_wobbly, (uint32_t)sizeof(utf8_wobbly)-1), mpack_error_invalid);

    // test UTF-8 cstr writers
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8_cstr(&writer, utf8_null)); // again, up to null-terminator, which is valid...
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8_cstr(&writer, utf8_valid));
    TEST_SIMPLE_WRITE_NOERROR(mpack_write_utf8_cstr(&writer, utf8_trimmed));
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_invalid), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_invalid_trimmed), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_truncated), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_modified), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_cesu8), mpack_error_invalid);
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr(&writer, utf8_wobbly), mpack_error_invalid);

    // some basic tests for utf8_cstr_or_nil
    TEST_SIMPLE_WRITE("\xa5hello", mpack_write_utf8_cstr_or_nil(&writer, "hello"));
    TEST_SIMPLE_WRITE("\xc0", mpack_write_utf8_cstr_or_nil(&writer, NULL));
    TEST_SIMPLE_WRITE_ERROR(mpack_write_utf8_cstr_or_nil(&writer, utf8_invalid), mpack_error_invalid);
}

typedef struct test_write_flush_t {
    char* out;
    size_t capacity;
    size_t count;
} test_write_flush_t;

static void test_write_flush_callback(mpack_writer_t* writer, const char* buffer, size_t count) {
    test_write_flush_t* flush = (test_write_flush_t*)writer->context;
    if (count > flush->capacity - flush->count) {
        mpack_writer_flag_error(writer, mpack_error_io);
        return;
    }
    memcpy(flush->out + flush->count, buffer, count);
    flush->count += count;
}

static void test_write_flush_message(void) {
    test_write_flush_t flush = {buf, sizeof(buf), 0};

    mpack_writer_t writer;
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_writer_set_context(&writer, &flush);
    mpack_writer_set_flush(&writer, &test_write_flush_callback);

    TEST_TRUE(flush.count == 0);
    mpack_write_cstr(&writer, "hello world!");
    TEST_TRUE(flush.count == 0);
    mpack_writer_flush_message(&writer);
    TEST_TRUE(flush.count == 13);

    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "a");
    mpack_write_int(&writer, 3);
    mpack_write_nil(&writer);
    mpack_write_true(&writer);
    mpack_finish_map(&writer);
    TEST_TRUE(flush.count == 13);
    mpack_writer_flush_message(&writer);
    TEST_TRUE(flush.count == 19);

    TEST_WRITER_DESTROY_NOERROR(&writer);

    // test break due to open message (if tracking is enabled)
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_writer_set_context(&writer, &flush);
    mpack_writer_set_flush(&writer, &test_write_flush_callback);
    mpack_start_map(&writer, 5);
    #if MPACK_WRITE_TRACKING
    TEST_BREAK((mpack_writer_flush_message(&writer), true));
    TEST_TRUE(mpack_writer_error(&writer) == mpack_error_bug);
    mpack_writer_flush_message(&writer); // no-op
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);
    #else
    mpack_writer_flush_message(&writer);
    TEST_WRITER_DESTROY_NOERROR(&writer);
    #endif

    // test break due to lack of flush
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_write_cstr(&writer, "hello world!");
    TEST_BREAK((mpack_writer_flush_message(&writer), true));
    TEST_TRUE(mpack_writer_error(&writer) == mpack_error_bug);
    mpack_writer_flush_message(&writer); // no-op
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);
}

static void test_misc(void) {

    // writing too much data without a flush callback
    // should result in mpack_error_too_big
    char shortbuf[10];
    mpack_writer_t writer;
    mpack_writer_init(&writer, shortbuf, sizeof(shortbuf));
    mpack_write_cstr(&writer, quick_brown_fox);
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_too_big);

    #if MPACK_STDLIB
    // writing strings larger than 32 bits should fail
    if (MPACK_UINT32_MAX < SIZE_MAX) {
        char single[1];

        mpack_writer_init(&writer, single, SIZE_MAX);
        test_system_mock_strlen((size_t)((uint64_t)MPACK_UINT32_MAX + MPACK_UINT64_C(1)));
        mpack_write_cstr(&writer, quick_brown_fox);
        TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_invalid);

        mpack_writer_init(&writer, single, SIZE_MAX);
        test_system_mock_strlen(SIZE_MAX);
        mpack_write_utf8_cstr(&writer, quick_brown_fox);
        TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_invalid);
    }
    #endif

}

#if MPACK_COMPATIBILITY
static void test_write_compatibility(void) {
    mpack_writer_t writer;

    // test str and bin behavior under all versions

    static const mpack_version_t versions[] = {
        mpack_version_v4,
        mpack_version_v5,
        mpack_version_current,
    };

    size_t i;
    for (i = 0; i < sizeof(versions) / sizeof(versions[0]); ++i) {
        mpack_version_t version = versions[i];
        mpack_writer_init(&writer, buf, sizeof(buf));
        mpack_writer_set_version(&writer, version);

        mpack_start_array(&writer, 2);
        mpack_write_cstr(&writer, quick_brown_fox);
        mpack_write_bin(&writer, quick_brown_fox, (uint32_t)strlen(quick_brown_fox));
        mpack_finish_array(&writer);

        size_t size = mpack_writer_buffer_used(&writer);
        TEST_WRITER_DESTROY_NOERROR(&writer);

        if (version == mpack_version_v4) {
            if (size != 1 + 6 + 2 * strlen(quick_brown_fox)) {
                TEST_TRUE(false, "incorrect length!");
            } else {
                TEST_TRUE(memcmp("\x92", buf, 1) == 0);
                TEST_TRUE(memcmp("\xda\x00\x2a", buf + 1, 3) == 0);
                TEST_TRUE(memcmp(quick_brown_fox, buf + 1 + 3, strlen(quick_brown_fox)) == 0);
                TEST_TRUE(memcmp("\xda\x00\x2a", buf + 1 + 3 + strlen(quick_brown_fox), 3) == 0);
                TEST_TRUE(memcmp(quick_brown_fox, buf + 1 + 3 + 3 + strlen(quick_brown_fox), strlen(quick_brown_fox)) == 0);
            }
        } else {
            if (size != 1 + 4 + 2 * strlen(quick_brown_fox)) {
                TEST_TRUE(false, "incorrect length!");
            } else {
                TEST_TRUE(memcmp("\x92", buf, 1) == 0);
                TEST_TRUE(memcmp("\xd9\x2a", buf + 1, 2) == 0);
                TEST_TRUE(memcmp(quick_brown_fox, buf + 1 + 2, strlen(quick_brown_fox)) == 0);
                TEST_TRUE(memcmp("\xc4\x2a", buf + 1 + 2 + strlen(quick_brown_fox), 2) == 0);
                TEST_TRUE(memcmp(quick_brown_fox, buf + 1 + 2 + 2 + strlen(quick_brown_fox), strlen(quick_brown_fox)) == 0);
            }
        }
    }

    #if MPACK_EXTENSIONS
    // test ext break in v4 mode
    mpack_writer_init(&writer, buf, sizeof(buf));
    mpack_writer_set_version(&writer, mpack_version_v4);
    TEST_BREAK(((void)mpack_start_ext(&writer, 1, 1), true));
    TEST_WRITER_DESTROY_ERROR(&writer, mpack_error_bug);
    #endif
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
    #if MPACK_HAS_GENERIC
    test_write_generic();
    test_write_generic_kv();
    #endif
    test_write_simple_misc();
    test_write_utf8();
    #if MPACK_EXTENSIONS
    test_write_timestamp();
    #endif

    #if MPACK_COMPATIBILITY
    test_write_compatibility();
    #endif

    #ifdef MPACK_MALLOC
    test_write_tag_tracking();
    test_write_basic_structures();
    test_write_small_structure_trees();
    test_system_fail_until_ok(&test_write_deep_growth);
    #endif

    #if MPACK_WRITE_TRACKING
    test_write_tracking();
    #endif

    test_write_flush_message();
    test_misc();
}

#endif

