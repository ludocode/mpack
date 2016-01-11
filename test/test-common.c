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

// we need internal to access the utf-8 check functions
#define MPACK_INTERNAL 1

#include "test-common.h"

#include <math.h>
#include "test.h"

static void test_tags_special(void) {

    // ensure there is only one inline definition (the global
    // address is in test.c)
    TEST_TRUE(fn_mpack_tag_nil == &mpack_tag_nil);

    // test comparison with invalid tags
    // (invalid enum values cause undefined behavior in C++)
    #if MPACK_DEBUG && !defined(__cplusplus)
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
    // (invalid enum values cause undefined behavior in C++)
    #if MPACK_DEBUG && !defined(__cplusplus)
    TEST_ASSERT(mpack_error_to_string((mpack_error_t)-1));
    TEST_ASSERT(mpack_type_to_string((mpack_type_t)-1));
    #endif
}

static void test_utf8_check(void) {
    #define EXPAND_STR_ARGS(str) str, sizeof(str) - 1


    // ascii
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\x00")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\x7F")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\x00""\x7F")));

    // nul
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\x00")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\x00""test")));
    TEST_TRUE(false == mpack_utf8_check_no_null(EXPAND_STR_ARGS("\x00")));
    TEST_TRUE(false == mpack_utf8_check_no_null(EXPAND_STR_ARGS("test\x00""test")));


    // 2-byte sequences
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\x80")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xDF\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\x80""test")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xDF\xBF""test")));

    // truncated 2-byte sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xDF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xDF")));

    // 2-byte overlong sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC0\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC1\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC0\xBF""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC1\xBF""test")));

    // not continuation bytes
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\x02")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\xC0")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\xE0")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\x02""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\xC0""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\xE0""test")));

    // miscellaneous 2-byte sequences
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\x80\xDF\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\x80""test\xDF\xBF""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\x70\xDF\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xC2\x80\xDF\xEF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\x00""test\xDF\xBF""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC2\x80""test\xDF\xEF""test")));


    // 3-byte sequences
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\xA0\x80")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xE7\xA0\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xEF\xBF\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\xA0\x80""test")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xE7\xA0\xBF""test")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xEF\xBF\xBF""test")));

    // truncated 3-byte sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xEF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE7\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xEA\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xEA\xBF")));

    // 3-byte overlong sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\x80\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\x9F\xFF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\x80\x80""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\x9F\xFF""test")));

    // not continuation bytes
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\x00\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\xF0\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\x80\x00")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\x80\xF0")));

    // surrogates
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xED\x9F\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xED\xA0\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xED\xBF\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xEE\x80\x80")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xED\x9F\xBF\xEE\x80\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xED\x9F\xBF\xED\xBF\xBF\xEE\x80\x80")));

    // miscellaneous 3-byte sequences
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\xA0\x80\xE7\xA0\xBF\xEF\xBF\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\xA0\x80\xE7\x00\xBF\xEF\xBF\xBF")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xE0\xA0\x80\xE7\xA0\xBF\xEF\xBF\x7F")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\xA0\x80""test\xE7\xA0\xBF""test\xEF\xBF\xBF""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\xA0\x80""test\xE7\xD0\xBF""test\xEF\xBF\xBF""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xE0\xA0\x80""test\xE7\xA0\xBF""test\xEF\x1F\xBF""test")));


    // 4-byte sequences
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x90\x80\x80"))); // U+10000
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xF4\x8F\xBF\xBF"))); // limit
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x90\x80\x80\xF4\x8F\xBF\xBF")));
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xF0\x90\x80\x80""test"))); // U+10000
    TEST_TRUE(true  == mpack_utf8_check(EXPAND_STR_ARGS("test\xF4\x8F\xBF\xBF""test"))); // limit

    // truncated 4-byte sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x90")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF1\x90\xB0")));

    // 4-byte overlong sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x80\x80\x80"))); // NUL
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x8F\xBF\xBF"))); // U+9999 (overlong)

    // not continuation bytes
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF0\x60\x80\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF1\x90\xD0\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF2\x90\x80\xF0")));

    // unicode limit
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF4\x90\x80\x80"))); // U+110000 (out of bounds)
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF6\x80\x80\x80")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("\xF7\x80\x80\x80")));


    // 5- and 6-byte sequences
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xF8\x80\x80\x80\x80""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xFB\x80\x80\x80\x80""test")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xFD\x80\x80\x80\x80\x80""test")));

    // other invalid bytes
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC0""testtesttest")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xC1""testtesttest")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xF5""testtesttest")));
    TEST_TRUE(false == mpack_utf8_check(EXPAND_STR_ARGS("test\xFF""testtesttest")));
}

void test_common() {
    test_tags_special();
    test_tags_simple();
    test_tags_reals();
    test_tags_compound();

    test_strings();
    test_utf8_check();
}

