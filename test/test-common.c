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

#include "test-common.h"

#include <math.h>
#include "test.h"

static void test_tags_special(void) {

    // ensure there is only one inline definition (the global
    // address is in test.c)
    TEST_TRUE(fn_mpack_tag_nil == &mpack_tag_nil);

    // test comparison with invalid tags
    // (invalid enum values are not allowed in C++)
    #ifndef __cplusplus
    mpack_tag_t invalid = mpack_tag_nil();
    invalid.type = (mpack_type_t)-1;
    TEST_ASSERT(mpack_tag_cmp(invalid, invalid));
    #endif

}

static void test_tags_simple(void) {

    // ensure tag types are correct
    TEST_TRUE(mpack_tag_nil().type == mpack_type_nil);
    TEST_TRUE(mpack_tag_bool(false).type == mpack_type_bool);
    TEST_TRUE(mpack_tag_int(0).type == mpack_type_int);
    TEST_TRUE(mpack_tag_uint(0).type == mpack_type_uint);

    // uints
    TEST_TRUE(mpack_tag_uint(0).v.u == 0);
    TEST_TRUE(mpack_tag_uint(1).v.u == 1);
    TEST_TRUE(mpack_tag_uint(INT32_MAX).v.u == INT32_MAX);
    TEST_TRUE(mpack_tag_uint(INT64_MAX).v.u == INT64_MAX);

    // ints
    TEST_TRUE(mpack_tag_int(0).v.i == 0);
    TEST_TRUE(mpack_tag_int(1).v.i == 1);
    // when using INT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_TRUE(mpack_tag_int(INT64_C(-2147483648)).v.i == INT64_C(-2147483648));
    TEST_TRUE(mpack_tag_int(INT64_MIN).v.i == INT64_MIN);

    // bools
    TEST_TRUE(mpack_tag_bool(true).v.b == true);
    TEST_TRUE(mpack_tag_bool(false).v.b == false);
    TEST_TRUE(mpack_tag_bool(1).v.b == true);
    TEST_TRUE(mpack_tag_bool(0).v.b == false);

    // comparisons of simple types
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_nil(), mpack_tag_nil()));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_nil(), mpack_tag_bool(false)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_nil(), mpack_tag_uint(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_int(0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_bool(false)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_bool(true), mpack_tag_bool(true)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bool(false), mpack_tag_bool(true)));

    // uint/int comparisons
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_uint(1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_uint(1), mpack_tag_uint(1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(-1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_int(-1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_int(-1), mpack_tag_int(-1)));

    // int to uint comparisons
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_uint(1), mpack_tag_int(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_uint(0), mpack_tag_int(1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_int(1), mpack_tag_uint(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_int(0), mpack_tag_uint(1)));

    // ordering

    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_uint(0), mpack_tag_uint(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_uint(1), mpack_tag_uint(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_int(-2), mpack_tag_int(-1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_int(-1), mpack_tag_int(-2)));

    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_str(0), mpack_tag_str(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_str(1), mpack_tag_str(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_bin(0), mpack_tag_bin(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_bin(1), mpack_tag_bin(0)));

    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_array(0), mpack_tag_array(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_array(1), mpack_tag_array(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_map(0), mpack_tag_map(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_map(1), mpack_tag_map(0)));

    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_ext(1, 1), mpack_tag_ext(2, 0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_ext(1, 1), mpack_tag_ext(1, 2)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_ext(2, 0), mpack_tag_ext(1, 1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_ext(1, 2), mpack_tag_ext(1, 1)));

}

static void test_tags_reals(void) {

    // types
    TEST_TRUE(mpack_tag_float(0.0f).type == mpack_type_float);
    TEST_TRUE(mpack_tag_double(0.0).type == mpack_type_double);
    TEST_TRUE(mpack_tag_float((float)NAN).type == mpack_type_float);
    TEST_TRUE(mpack_tag_double((double)NAN).type == mpack_type_double);

    // float comparisons
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_float(1), mpack_tag_float(1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(MPACK_INFINITY)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_float(-MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(1), mpack_tag_float(MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(0), mpack_tag_float(NAN)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(MPACK_INFINITY), mpack_tag_float(NAN)));

    // double comparisons
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_double(1), mpack_tag_double(1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(MPACK_INFINITY)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_double(-MPACK_INFINITY), mpack_tag_double(-MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(1), mpack_tag_double(MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(-MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_double(NAN)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_double(NAN)));

    // float/double comparisons
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(0), mpack_tag_float(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(1), mpack_tag_float(1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(MPACK_INFINITY), mpack_tag_float(MPACK_INFINITY)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_double(-MPACK_INFINITY), mpack_tag_float(-MPACK_INFINITY)));

    // here we're comparing NaNs and we expect true. this is because we compare
    // floats bit-for-bit, not using operator==
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_float(NAN), mpack_tag_float(NAN)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_double(NAN), mpack_tag_double(NAN)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_float(NAN), mpack_tag_double(NAN)));

}

static void test_tags_compound() {
    TEST_TRUE(mpack_tag_array(0).type == mpack_type_array);
    TEST_TRUE(mpack_tag_map(0).type == mpack_type_map);
    TEST_TRUE(mpack_tag_str(0).type == mpack_type_str);
    TEST_TRUE(mpack_tag_bin(0).type == mpack_type_bin);
    TEST_TRUE(mpack_tag_ext(0, 0).type == mpack_type_ext);

    TEST_TRUE(true == mpack_tag_equal(mpack_tag_array(0), mpack_tag_array(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_array(0), mpack_tag_array(1)));
    TEST_TRUE(0 == mpack_tag_cmp(mpack_tag_array(0), mpack_tag_array(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_array(0), mpack_tag_array(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_array(1), mpack_tag_array(0)));

    TEST_TRUE(true == mpack_tag_equal(mpack_tag_map(0), mpack_tag_map(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_map(0), mpack_tag_map(1)));
    TEST_TRUE(0 == mpack_tag_cmp(mpack_tag_map(0), mpack_tag_map(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_map(0), mpack_tag_map(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_map(1), mpack_tag_map(0)));

    TEST_TRUE(true == mpack_tag_equal(mpack_tag_str(0), mpack_tag_str(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_str(0), mpack_tag_str(1)));
    TEST_TRUE(0 == mpack_tag_cmp(mpack_tag_str(0), mpack_tag_str(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_str(0), mpack_tag_str(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_str(1), mpack_tag_str(0)));

    TEST_TRUE(true == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_bin(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_bin(1)));
    TEST_TRUE(0 == mpack_tag_cmp(mpack_tag_bin(0), mpack_tag_bin(0)));
    TEST_TRUE(-1 == mpack_tag_cmp(mpack_tag_bin(0), mpack_tag_bin(1)));
    TEST_TRUE(1 == mpack_tag_cmp(mpack_tag_bin(1), mpack_tag_bin(0)));

    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_ext(0, 0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(0, 1), mpack_tag_ext(0, 1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(127, 0), mpack_tag_ext(127, 0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(127, 1), mpack_tag_ext(127, 1)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(-128, 0), mpack_tag_ext(-128, 0)));
    TEST_TRUE(true == mpack_tag_equal(mpack_tag_ext(-128, 1), mpack_tag_ext(-128, 1)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_ext(127, 0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_ext(-128, 0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_ext(0, 1)));

    TEST_TRUE(false == mpack_tag_equal(mpack_tag_array(0), mpack_tag_map(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_array(0), mpack_tag_str(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_array(0), mpack_tag_bin(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_array(0), mpack_tag_ext(0, 0)));

    TEST_TRUE(false == mpack_tag_equal(mpack_tag_map(0), mpack_tag_array(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_map(0), mpack_tag_str(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_map(0), mpack_tag_bin(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_map(0), mpack_tag_ext(0, 0)));

    TEST_TRUE(false == mpack_tag_equal(mpack_tag_str(0), mpack_tag_array(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_str(0), mpack_tag_map(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_str(0), mpack_tag_bin(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_str(0), mpack_tag_ext(0, 0)));

    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_array(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_map(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_str(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_bin(0), mpack_tag_ext(0, 0)));

    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_array(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_map(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_str(0)));
    TEST_TRUE(false == mpack_tag_equal(mpack_tag_ext(0, 0), mpack_tag_bin(0)));
}

static void test_string(const char* str, const char* content) {
    #if MPACK_DEBUG
    // in debug mode, the string should contain the expected content
    TEST_TRUE(strstr(str, content) != NULL, "string \"%s\" does not contain \"%s\"", str, content);
    #else
    // in release mode, strings should be blank
    MPACK_UNUSED(content);
    TEST_TRUE(strcmp(str, "") == 0, "string is not empty: %s", str);
    #endif
}

static void test_strings() {
    test_string(mpack_error_to_string(mpack_ok), "ok");
    #define TEST_ERROR_STRING(error) test_string(mpack_error_to_string(mpack_error_##error), #error)
    TEST_ERROR_STRING(io);
    TEST_ERROR_STRING(invalid);
    TEST_ERROR_STRING(type);
    TEST_ERROR_STRING(too_big);
    TEST_ERROR_STRING(memory);
    TEST_ERROR_STRING(bug);
    TEST_ERROR_STRING(data);
    #undef TEST_ERROR_STRING

    #define TEST_ERROR_TYPE(type) test_string(mpack_type_to_string(mpack_type_##type), #type)
    TEST_ERROR_TYPE(nil);
    TEST_ERROR_TYPE(bool);
    TEST_ERROR_TYPE(float);
    TEST_ERROR_TYPE(double);
    TEST_ERROR_TYPE(int);
    TEST_ERROR_TYPE(uint);
    TEST_ERROR_TYPE(str);
    TEST_ERROR_TYPE(bin);
    TEST_ERROR_TYPE(ext);
    TEST_ERROR_TYPE(array);
    TEST_ERROR_TYPE(map);
    #undef TEST_ERROR_TYPE

    // test strings for invalid enum values
    // (invalid enum values are not allowed in C++)
    #ifndef __cplusplus
    TEST_ASSERT(mpack_error_to_string((mpack_error_t)-1));
    TEST_ASSERT(mpack_type_to_string((mpack_type_t)-1));
    #endif
}

void test_common() {
    test_tags_special();
    test_tags_simple();
    test_tags_reals();
    test_tags_compound();

    test_strings();
}

