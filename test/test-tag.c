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

#include "test-tag.h"

#include <math.h>
#include "test.h"

void test_tags() {

    // ensure there is only one inline definition (the other
    // address here is in main)
    test_assert(fn_mpack_tag_nil == &mpack_tag_nil);

    // uints
    test_assert(mpack_tag_uint(0).v.u == 0);
    test_assert(mpack_tag_uint(1).v.u == 1);
    test_assert(mpack_tag_uint(INT32_MAX).v.u == INT32_MAX);
    test_assert(mpack_tag_uint(INT64_MAX).v.u == INT64_MAX);

    // ints
    test_assert(mpack_tag_int(0).v.i == 0);
    test_assert(mpack_tag_int(1).v.i == 1);
    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    test_assert(mpack_tag_int(INT64_C(-2147483648)).v.i == INT64_C(-2147483648));
    test_assert(mpack_tag_int(INT64_MIN).v.i == INT64_MIN);

    // bools
    test_assert(mpack_tag_bool(true).v.b == true);
    test_assert(mpack_tag_bool(false).v.b == false);
    test_assert(mpack_tag_bool(1).v.b == true);
    test_assert(mpack_tag_bool(0).v.b == false);

    // comparisons of simple types
    test_assert(true == mpack_tag_equal(mpack_tag_nil(), mpack_tag_nil()));
    test_assert(false == mpack_tag_equal(mpack_tag_nil(), mpack_tag_bool(false)));
    test_assert(false == mpack_tag_equal(mpack_tag_nil(), mpack_tag_uint(0)));
    test_assert(false == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_int(0)));
    test_assert(true == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_bool(false)));
    test_assert(true == mpack_tag_equal(mpack_tag_bool(true), mpack_tag_bool(true)));
    test_assert(false == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_bool(true)));

    // uint/int comparisons
    test_assert(true == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(0)));
    test_assert(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(1)));
    test_assert(true == mpack_tag_equal(mpack_tag_uint(1), mpack_tag_uint(1)));
    test_assert(true == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(0)));
    test_assert(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(-1)));
    test_assert(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(-1)));
    test_assert(true == mpack_tag_equal(mpack_tag_int(-1), mpack_tag_int(-1)));

    // int to uint comparisons
    test_assert(true == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(0)));
    test_assert(true == mpack_tag_equal(mpack_tag_uint(1), mpack_tag_int(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(1)));
    test_assert(true == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(0)));
    test_assert(true == mpack_tag_equal(mpack_tag_int(1), mpack_tag_uint(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(1)));

    // float comparisons
    test_assert(true == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(0)));
    test_assert(true == mpack_tag_equal(mpack_tag_float(1), mpack_tag_float(1)));
    test_assert(true == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(MPACK_INFINITY)));
    test_assert(true == mpack_tag_equal(mpack_tag_float(-MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(1), mpack_tag_float(MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(NAN)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(NAN)));

    // double comparisons
    test_assert(true == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(0)));
    test_assert(true == mpack_tag_equal(mpack_tag_double(1), mpack_tag_double(1)));
    test_assert(true == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(MPACK_INFINITY)));
    test_assert(true == mpack_tag_equal(mpack_tag_double(-MPACK_INFINITY), mpack_tag_double(-MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(1), mpack_tag_double(MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(-MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(NAN)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(NAN)));

    // float/double comparisons
    test_assert(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_float(0)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(1), mpack_tag_float(1)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_float(MPACK_INFINITY)));
    test_assert(false == mpack_tag_equal(mpack_tag_double(-MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));

    // here we're comparing NaNs and we expect true. this is because we compare
    // floats bit-for-bit, not using operator==
    test_assert(true == mpack_tag_equal(mpack_tag_float(NAN), mpack_tag_float(NAN)));
    test_assert(true == mpack_tag_equal(mpack_tag_double(NAN), mpack_tag_double(NAN)));
    test_assert(false == mpack_tag_equal(mpack_tag_float(NAN), mpack_tag_double(NAN)));
}

