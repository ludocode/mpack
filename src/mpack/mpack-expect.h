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

/**
 * @file
 *
 * Declares the MPack static Expect API.
 */

#ifndef MPACK_EXPECT_H
#define MPACK_EXPECT_H 1

#include "mpack-reader.h"

MPACK_HEADER_START

#if MPACK_EXPECT

#if !MPACK_READER
#error "MPACK_EXPECT requires MPACK_READER."
#endif

/**
 * @defgroup expect Expect API
 *
 * The MPack Expect API allows you to easily read MessagePack data when you
 * expect it to follow a predefined schema.
 *
 * See @ref docs/expect.md for examples.
 *
 * The main purpose of the Expect API is convenience, so the API is lax. It
 * automatically converts between similar types where there is no loss of
 * precision.
 *
 * When using any of the expect functions, if the type or value of what was
 * read does not match what is expected, @ref mpack_error_type is raised.
 *
 * @{
 */

/**
 * @name Basic Number Functions
 * @{
 */

/**
 * Reads an 8-bit unsigned integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit unsigned int.
 *
 * Returns zero if an error occurs.
 */
uint8_t mpack_expect_u8(mpack_reader_t* reader);

/**
 * Reads a 16-bit unsigned integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit unsigned int.
 *
 * Returns zero if an error occurs.
 */
uint16_t mpack_expect_u16(mpack_reader_t* reader);

/**
 * Reads a 32-bit unsigned integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit unsigned int.
 *
 * Returns zero if an error occurs.
 */
uint32_t mpack_expect_u32(mpack_reader_t* reader);

/**
 * Reads a 64-bit unsigned integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit unsigned int.
 *
 * Returns zero if an error occurs.
 */
uint64_t mpack_expect_u64(mpack_reader_t* reader);

/**
 * Reads an 8-bit signed integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit signed int.
 *
 * Returns zero if an error occurs.
 */
int8_t mpack_expect_i8(mpack_reader_t* reader);

/**
 * Reads a 16-bit signed integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit signed int.
 *
 * Returns zero if an error occurs.
 */
int16_t mpack_expect_i16(mpack_reader_t* reader);

/**
 * Reads a 32-bit signed integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit signed int.
 *
 * Returns zero if an error occurs.
 */
int32_t mpack_expect_i32(mpack_reader_t* reader);

/**
 * Reads a 64-bit signed integer.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit signed int.
 *
 * Returns zero if an error occurs.
 */
int64_t mpack_expect_i64(mpack_reader_t* reader);

/**
 * Reads a number, returning the value as a float. The underlying value can be an
 * integer, float or double; the value is converted to a float.
 *
 * @note Reading a double or a large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
float mpack_expect_float(mpack_reader_t* reader);

/**
 * Reads a number, returning the value as a double. The underlying value can be an
 * integer, float or double; the value is converted to a double.
 *
 * @note Reading a very large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
double mpack_expect_double(mpack_reader_t* reader);

/**
 * Reads a float. The underlying value must be a float, not a double or an integer.
 * This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float.
 */
float mpack_expect_float_strict(mpack_reader_t* reader);

/**
 * Reads a double. The underlying value must be a float or double, not an integer.
 * This ensures no loss of precision can occur.
 *
 * @throws mpack_error_type if the underlying value is not a float or double.
 */
double mpack_expect_double_strict(mpack_reader_t* reader);

/**
 * @}
 */

/**
 * @name Ranged Number Functions
 * @{
 */

/**
 * Reads an 8-bit unsigned integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit unsigned int.
 *
 * Returns min_value if an error occurs.
 */
uint8_t mpack_expect_u8_range(mpack_reader_t* reader, uint8_t min_value, uint8_t max_value);

/**
 * Reads a 16-bit unsigned integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit unsigned int.
 *
 * Returns min_value if an error occurs.
 */
uint16_t mpack_expect_u16_range(mpack_reader_t* reader, uint16_t min_value, uint16_t max_value);

/**
 * Reads a 32-bit unsigned integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit unsigned int.
 *
 * Returns min_value if an error occurs.
 */
uint32_t mpack_expect_u32_range(mpack_reader_t* reader, uint32_t min_value, uint32_t max_value);

/**
 * Reads a 64-bit unsigned integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit unsigned int.
 *
 * Returns min_value if an error occurs.
 */
uint64_t mpack_expect_u64_range(mpack_reader_t* reader, uint64_t min_value, uint64_t max_value);

/**
 * Reads an unsigned integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an unsigned int.
 *
 * Returns min_value if an error occurs.
 */
MPACK_INLINE unsigned int mpack_expect_uint_range(mpack_reader_t* reader, unsigned int min_value, unsigned int max_value) {
    // This should be true at compile-time, so this just wraps the 32-bit
    // function. We fallback to 64-bit if for some reason sizeof(int) isn't 4.
    if (sizeof(unsigned int) == 4)
        return (unsigned int)mpack_expect_u32_range(reader, (uint32_t)min_value, (uint32_t)max_value);
    return (unsigned int)mpack_expect_u64_range(reader, min_value, max_value);
}

/**
 * Reads an 8-bit unsigned integer, ensuring that it is at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit unsigned int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE uint8_t mpack_expect_u8_max(mpack_reader_t* reader, uint8_t max_value) {
    return mpack_expect_u8_range(reader, 0, max_value);
}

/**
 * Reads a 16-bit unsigned integer, ensuring that it is at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit unsigned int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE uint16_t mpack_expect_u16_max(mpack_reader_t* reader, uint16_t max_value) {
    return mpack_expect_u16_range(reader, 0, max_value);
}

/**
 * Reads a 32-bit unsigned integer, ensuring that it is at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit unsigned int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE uint32_t mpack_expect_u32_max(mpack_reader_t* reader, uint32_t max_value) {
    return mpack_expect_u32_range(reader, 0, max_value);
}

/**
 * Reads a 64-bit unsigned integer, ensuring that it is at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit unsigned int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE uint64_t mpack_expect_u64_max(mpack_reader_t* reader, uint64_t max_value) {
    return mpack_expect_u64_range(reader, 0, max_value);
}

/**
 * Reads an unsigned integer, ensuring that it is at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an unsigned int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE unsigned int mpack_expect_uint_max(mpack_reader_t* reader, unsigned int max_value) {
    return mpack_expect_uint_range(reader, 0, max_value);
}

/**
 * Reads an 8-bit signed integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit signed int.
 *
 * Returns min_value if an error occurs.
 */
int8_t mpack_expect_i8_range(mpack_reader_t* reader, int8_t min_value, int8_t max_value);

/**
 * Reads a 16-bit signed integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit signed int.
 *
 * Returns min_value if an error occurs.
 */
int16_t mpack_expect_i16_range(mpack_reader_t* reader, int16_t min_value, int16_t max_value);

/**
 * Reads a 32-bit signed integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit signed int.
 *
 * Returns min_value if an error occurs.
 */
int32_t mpack_expect_i32_range(mpack_reader_t* reader, int32_t min_value, int32_t max_value);

/**
 * Reads a 64-bit signed integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit signed int.
 *
 * Returns min_value if an error occurs.
 */
int64_t mpack_expect_i64_range(mpack_reader_t* reader, int64_t min_value, int64_t max_value);

/**
 * Reads a signed integer, ensuring that it falls within the given range.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a signed int.
 *
 * Returns min_value if an error occurs.
 */
MPACK_INLINE int mpack_expect_int_range(mpack_reader_t* reader, int min_value, int max_value) {
    // This should be true at compile-time, so this just wraps the 32-bit
    // function. We fallback to 64-bit if for some reason sizeof(int) isn't 4.
    if (sizeof(int) == 4)
        return (int)mpack_expect_i32_range(reader, (int32_t)min_value, (int32_t)max_value);
    return (int)mpack_expect_i64_range(reader, min_value, max_value);
}

/**
 * Reads an 8-bit signed integer, ensuring that it is at least zero and at
 * most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an 8-bit signed int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE int8_t mpack_expect_i8_max(mpack_reader_t* reader, int8_t max_value) {
    return mpack_expect_i8_range(reader, 0, max_value);
}

/**
 * Reads a 16-bit signed integer, ensuring that it is at least zero and at
 * most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 16-bit signed int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE int16_t mpack_expect_i16_max(mpack_reader_t* reader, int16_t max_value) {
    return mpack_expect_i16_range(reader, 0, max_value);
}

/**
 * Reads a 32-bit signed integer, ensuring that it is at least zero and at
 * most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 32-bit signed int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE int32_t mpack_expect_i32_max(mpack_reader_t* reader, int32_t max_value) {
    return mpack_expect_i32_range(reader, 0, max_value);
}

/**
 * Reads a 64-bit signed integer, ensuring that it is at least zero and at
 * most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a 64-bit signed int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE int64_t mpack_expect_i64_max(mpack_reader_t* reader, int64_t max_value) {
    return mpack_expect_i64_range(reader, 0, max_value);
}

/**
 * Reads an int, ensuring that it is at least zero and at most @a max_value.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a signed int.
 *
 * Returns 0 if an error occurs.
 */
MPACK_INLINE int mpack_expect_int_max(mpack_reader_t* reader, int max_value) {
    return mpack_expect_int_range(reader, 0, max_value);
}

/**
 * Reads a number, ensuring that it falls within the given range and returning
 * the value as a float. The underlying value can be an integer, float or
 * double; the value is converted to a float.
 *
 * @note Reading a double or a large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
float mpack_expect_float_range(mpack_reader_t* reader, float min_value, float max_value);

/**
 * Reads a number, ensuring that it falls within the given range and returning
 * the value as a double. The underlying value can be an integer, float or
 * double; the value is converted to a double.
 *
 * @note Reading a very large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
double mpack_expect_double_range(mpack_reader_t* reader, double min_value, double max_value);

/**
 * @}
 */



// These are additional Basic Number functions that wrap inline range functions.

/**
 * @name Basic Number Functions
 * @{
 */

/**
 * Reads an unsigned int.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in an unsigned int.
 *
 * Returns zero if an error occurs.
 */
MPACK_INLINE unsigned int mpack_expect_uint(mpack_reader_t* reader) {

    // This should be true at compile-time, so this just wraps the 32-bit function.
    if (sizeof(unsigned int) == 4)
        return (unsigned int)mpack_expect_u32(reader);

    // Otherwise we wrap the max function to ensure it fits.
    return (unsigned int)mpack_expect_u64_max(reader, UINT_MAX);

}

/**
 * Reads a signed int.
 *
 * The underlying type may be an integer type of any size and signedness,
 * as long as the value can be represented in a signed int.
 *
 * Returns zero if an error occurs.
 */
MPACK_INLINE int mpack_expect_int(mpack_reader_t* reader) {

    // This should be true at compile-time, so this just wraps the 32-bit function.
    if (sizeof(int) == 4)
        return (int)mpack_expect_i32(reader);

    // Otherwise we wrap the range function to ensure it fits.
    return (int)mpack_expect_i64_range(reader, INT_MIN, INT_MAX);

}

/**
 * @}
 */



/**
 * @name Matching Number Functions
 * @{
 */

/**
 * Reads an unsigned integer, ensuring that it exactly matches the given value.
 *
 * mpack_error_type is raised if the value is not representable as an unsigned
 * integer or if it does not exactly match the given value.
 */
void mpack_expect_uint_match(mpack_reader_t* reader, uint64_t value);

/**
 * Reads a signed integer, ensuring that it exactly matches the given value.
 *
 * mpack_error_type is raised if the value is not representable as a signed
 * integer or if it does not exactly match the given value.
 */
void mpack_expect_int_match(mpack_reader_t* reader, int64_t value);

/**
 * @name Other Basic Types
 * @{
 */

/**
 * Reads a nil, raising @ref mpack_error_type if the value is not nil.
 */
void mpack_expect_nil(mpack_reader_t* reader);

/**
 * Reads a boolean.
 *
 * @note Integers will raise mpack_error_type; the value must be strictly a boolean.
 */
bool mpack_expect_bool(mpack_reader_t* reader);

/**
 * Reads a boolean, raising @ref mpack_error_type if its value is not @c true.
 */
void mpack_expect_true(mpack_reader_t* reader);

/**
 * Reads a boolean, raising @ref mpack_error_type if its value is not @c false.
 */
void mpack_expect_false(mpack_reader_t* reader);


/**
 * @}
 */

/**
 * @name Compound Types
 * @{
 */

/**
 * Reads the start of a map, returning its element count.
 *
 * A number of values follow equal to twice the element count of the map,
 * alternating between keys and values. @ref mpack_done_map() must be called
 * once all elements have been read.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * @warning This call is dangerous! It does not have a size limit, and it
 * does not have any way of checking whether there is enough data in the
 * message (since the data could be coming from a stream.) When looping
 * through the map's contents, you must check for errors on each iteration
 * of the loop. Otherwise an attacker could craft a message declaring a map
 * of a billion elements which would throw your parsing code into an
 * infinite loop! You should strongly consider using mpack_expect_map_max()
 * with a safe maximum size instead.
 *
 * @throws mpack_error_type if the value is not a map.
 */
uint32_t mpack_expect_map(mpack_reader_t* reader);

/**
 * Reads the start of a map with a number of elements in the given range, returning
 * its element count.
 *
 * A number of values follow equal to twice the element count of the map,
 * alternating between keys and values. @ref mpack_done_map() must be called
 * once all elements have been read.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * min_count is returned if an error occurs.
 *
 * @throws mpack_error_type if the value is not a map or if its size does
 * not fall within the given range.
 */
uint32_t mpack_expect_map_range(mpack_reader_t* reader, uint32_t min_count, uint32_t max_count);

/**
 * Reads the start of a map with a number of elements at most @a max_count,
 * returning its element count.
 *
 * A number of values follow equal to twice the element count of the map,
 * alternating between keys and values. @ref mpack_done_map() must be called
 * once all elements have been read.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * Zero is returned if an error occurs.
 *
 * @throws mpack_error_type if the value is not a map or if its size is
 * greater than max_count.
 */
MPACK_INLINE uint32_t mpack_expect_map_max(mpack_reader_t* reader, uint32_t max_count) {
    return mpack_expect_map_range(reader, 0, max_count);
}

/**
 * Reads the start of a map of the exact size given.
 *
 * A number of values follow equal to twice the element count of the map,
 * alternating between keys and values. @ref mpack_done_map() must be called
 * once all elements have been read.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * @throws mpack_error_type if the value is not a map or if its size
 * does not match the given count.
 */
void mpack_expect_map_match(mpack_reader_t* reader, uint32_t count);

/**
 * Reads a nil node or the start of a map, returning whether a map was
 * read and placing its number of key/value pairs in count.
 *
 * If a map was read, a number of values follow equal to twice the element count
 * of the map, alternating between keys and values. @ref mpack_done_map() should
 * also be called once all elements have been read (only if a map was read.)
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * @warning This call is dangerous! It does not have a size limit, and it
 * does not have any way of checking whether there is enough data in the
 * message (since the data could be coming from a stream.) When looping
 * through the map's contents, you must check for errors on each iteration
 * of the loop. Otherwise an attacker could craft a message declaring a map
 * of a billion elements which would throw your parsing code into an
 * infinite loop! You should strongly consider using mpack_expect_map_max_or_nil()
 * with a safe maximum size instead.
 *
 * @returns @c true if a map was read successfully; @c false if nil was read
 *     or an error occured.
 * @throws mpack_error_type if the value is not a nil or map.
 */
bool mpack_expect_map_or_nil(mpack_reader_t* reader, uint32_t* count);

/**
 * Reads a nil node or the start of a map with a number of elements at most
 * max_count, returning whether a map was read and placing its number of
 * key/value pairs in count.
 *
 * If a map was read, a number of values follow equal to twice the element count
 * of the map, alternating between keys and values. @ref mpack_done_map() should
 * anlso be called once all elements have been read (only if a map was read.)
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON. Consider using mpack_expect_key_cstr() or mpack_expect_key_uint()
 * to switch on the key; see @ref docs/expect.md for examples.
 *
 * @returns @c true if a map was read successfully; @c false if nil was read
 *     or an error occured.
 * @throws mpack_error_type if the value is not a nil or map.
 */
bool mpack_expect_map_max_or_nil(mpack_reader_t* reader, uint32_t max_count, uint32_t* count);

/**
 * Reads the start of an array, returning its element count.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * @warning This call is dangerous! It does not have a size limit, and it
 * does not have any way of checking whether there is enough data in the
 * message (since the data could be coming from a stream.) When looping
 * through the array's contents, you must check for errors on each iteration
 * of the loop. Otherwise an attacker could craft a message declaring an array
 * of a billion elements which would throw your parsing code into an
 * infinite loop! You should strongly consider using mpack_expect_array_max()
 * with a safe maximum size instead.
 */
uint32_t mpack_expect_array(mpack_reader_t* reader);

/**
 * Reads the start of an array with a number of elements in the given range,
 * returning its element count.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * min_count is returned if an error occurs.
 *
 * @throws mpack_error_type if the value is not an array or if its size does
 * not fall within the given range.
 */
uint32_t mpack_expect_array_range(mpack_reader_t* reader, uint32_t min_count, uint32_t max_count);

/**
 * Reads the start of an array with a number of elements at most @a max_count,
 * returning its element count.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * Zero is returned if an error occurs.
 *
 * @throws mpack_error_type if the value is not an array or if its size is
 * greater than max_count.
 */
MPACK_INLINE uint32_t mpack_expect_array_max(mpack_reader_t* reader, uint32_t max_count) {
    return mpack_expect_array_range(reader, 0, max_count);
}

/**
 * Reads the start of an array of the exact size given.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * @throws mpack_error_type if the value is not an array or if its size does
 * not match the given count.
 */
void mpack_expect_array_match(mpack_reader_t* reader, uint32_t count);

/**
 * Reads a nil node or the start of an array, returning whether an array was
 * read and placing its number of elements in count.
 *
 * If an array was read, a number of values follow equal to the element count
 * of the array. @ref mpack_done_array() should also be called once all elements
 * have been read (only if an array was read.)
 *
 * @warning This call is dangerous! It does not have a size limit, and it
 * does not have any way of checking whether there is enough data in the
 * message (since the data could be coming from a stream.) When looping
 * through the array's contents, you must check for errors on each iteration
 * of the loop. Otherwise an attacker could craft a message declaring an array
 * of a billion elements which would throw your parsing code into an
 * infinite loop! You should strongly consider using mpack_expect_array_max_or_nil()
 * with a safe maximum size instead.
 *
 * @returns @c true if an array was read successfully; @c false if nil was read
 *     or an error occured.
 * @throws mpack_error_type if the value is not a nil or array.
 */
bool mpack_expect_array_or_nil(mpack_reader_t* reader, uint32_t* count);

/**
 * Reads a nil node or the start of an array with a number of elements at most
 * max_count, returning whether an array was read and placing its number of
 * key/value pairs in count.
 *
 * If an array was read, a number of values follow equal to the element count
 * of the array. @ref mpack_done_array() should also be called once all elements
 * have been read (only if an array was read.)
 *
 * @returns @c true if an array was read successfully; @c false if nil was read
 *     or an error occured.
 * @throws mpack_error_type if the value is not a nil or array.
 */
bool mpack_expect_array_max_or_nil(mpack_reader_t* reader, uint32_t max_count, uint32_t* count);

#ifdef MPACK_MALLOC
/**
 * @hideinitializer
 *
 * Reads the start of an array and allocates storage for it, placing its
 * size in out_count. A number of objects follow equal to the element count
 * of the array. You must call @ref mpack_done_array() when done (even
 * if the element count is zero.)
 *
 * If an error occurs, NULL is returned and the reader is placed in an
 * error state.
 *
 * If the count is zero, NULL is returned. This does not indicate error.
 * You should not check the return value for NULL to check for errors; only
 * check the reader's error state.
 *
 * The allocated array must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 *
 * @throws mpack_error_type if the value is not an array or if its size is
 * greater than max_count.
 */
#define mpack_expect_array_alloc(reader, Type, max_count, out_count) \
    ((Type*)mpack_expect_array_alloc_impl(reader, sizeof(Type), max_count, out_count, false))

/**
 * @hideinitializer
 *
 * Reads a nil node or the start of an array and allocates storage for it,
 * placing its size in out_count. A number of objects follow equal to the element
 * count of the array if a non-empty array was read.
 *
 * If an error occurs, NULL is returned and the reader is placed in an
 * error state.
 *
 * If a nil node was read, NULL is returned. If an empty array was read,
 * mpack_done_array() is called automatically and NULL is returned. These
 * do not indicate error. You should not check the return value for NULL
 * to check for errors; only check the reader's error state.
 *
 * The allocated array must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 *
 * @warning You must call @ref mpack_done_array() if and only if a non-zero
 * element count is read. This function does not differentiate between nil
 * and an empty array.
 *
 * @throws mpack_error_type if the value is not an array or if its size is
 * greater than max_count.
 */
#define mpack_expect_array_or_nil_alloc(reader, Type, max_count, out_count) \
    ((Type*)mpack_expect_array_alloc_impl(reader, sizeof(Type), max_count, out_count, true))
#endif

/**
 * @}
 */

/** @cond */
#ifdef MPACK_MALLOC
void* mpack_expect_array_alloc_impl(mpack_reader_t* reader,
        size_t element_size, uint32_t max_count, uint32_t* out_count, bool allow_nil);
#endif
/** @endcond */


/**
 * @name String Functions
 * @{
 */

/**
 * Reads the start of a string, returning its size in bytes.
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). mpack_done_str() must be called
 * once all bytes have been read.
 *
 * NUL bytes are allowed in the string, and no encoding checks are done.
 *
 * mpack_error_type is raised if the value is not a string.
 */
uint32_t mpack_expect_str(mpack_reader_t* reader);

/**
 * Reads a string of at most the given size, writing it into the
 * given buffer and returning its size in bytes.
 *
 * This does not add a null-terminator! Use mpack_expect_cstr() to
 * add a null-terminator.
 *
 * NUL bytes are allowed in the string, and no encoding checks are done.
 */
size_t mpack_expect_str_buf(mpack_reader_t* reader, char* buf, size_t bufsize);

/**
 * Reads a string into the given buffer, ensuring it is a valid UTF-8 string
 * and returning its size in bytes.
 *
 * This does not add a null-terminator! Use mpack_expect_utf8_cstr() to
 * add a null-terminator.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * NUL bytes are allowed in the string (as they are in UTF-8.)
 *
 * Raises mpack_error_too_big if there is not enough room for the string.
 * Raises mpack_error_type if the value is not a string or is not a valid UTF-8 string.
 */
size_t mpack_expect_utf8(mpack_reader_t* reader, char* buf, size_t bufsize);

/**
 * Reads the start of a string, raising an error if its length is not
 * at most the given number of bytes (not including any null-terminator.)
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_str() must be called
 * once all bytes have been read.
 *
 * @throws mpack_error_type If the value is not a string.
 * @throws mpack_error_too_big If the string's length in bytes is larger than the given maximum size.
 */
MPACK_INLINE uint32_t mpack_expect_str_max(mpack_reader_t* reader, uint32_t maxsize) {
    uint32_t length = mpack_expect_str(reader);
    if (length > maxsize) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return 0;
    }
    return length;
}

/**
 * Reads the start of a string, raising an error if its length is not
 * exactly the given number of bytes (not including any null-terminator.)
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_str() must be called
 * once all bytes have been read.
 *
 * mpack_error_type is raised if the value is not a string or if its
 * length does not match.
 */
MPACK_INLINE void mpack_expect_str_length(mpack_reader_t* reader, uint32_t count) {
    if (mpack_expect_str(reader) != count)
        mpack_reader_flag_error(reader, mpack_error_type);
}

/**
 * Reads a string, ensuring it exactly matches the given string.
 *
 * Remember that maps are unordered in JSON. Don't use this for map keys
 * unless the map has only a single key!
 */
void mpack_expect_str_match(mpack_reader_t* reader, const char* str, size_t length);

/**
 * Reads a string into the given buffer, ensures it has no null bytes,
 * and adds a null-terminator at the end.
 *
 * Raises mpack_error_too_big if there is not enough room for the string and null-terminator.
 * Raises mpack_error_type if the value is not a string or contains a null byte.
 */
void mpack_expect_cstr(mpack_reader_t* reader, char* buf, size_t size);

/**
 * Reads a string into the given buffer, ensures it is a valid UTF-8 string
 * without NUL characters, and adds a null-terminator at the end.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed, but without the NUL character, since
 * it cannot be represented in a null-terminated string.
 *
 * Raises mpack_error_too_big if there is not enough room for the string and null-terminator.
 * Raises mpack_error_type if the value is not a string or is not a valid UTF-8 string.
 */
void mpack_expect_utf8_cstr(mpack_reader_t* reader, char* buf, size_t size);

#ifdef MPACK_MALLOC
/**
 * Reads a string with the given total maximum size (including space for a
 * null-terminator), allocates storage for it, ensures it has no null-bytes,
 * and adds a null-terminator at the end. You assume ownership of the
 * returned pointer if reading succeeds.
 *
 * The allocated string must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 *
 * @throws mpack_error_too_big If the string plus null-terminator is larger than the given maxsize.
 * @throws mpack_error_type If the value is not a string or contains a null byte.
 */
char* mpack_expect_cstr_alloc(mpack_reader_t* reader, size_t maxsize);

/**
 * Reads a string with the given total maximum size (including space for a
 * null-terminator), allocates storage for it, ensures it is valid UTF-8
 * with no null-bytes, and adds a null-terminator at the end. You assume
 * ownership of the returned pointer if reading succeeds.
 *
 * The length in bytes of the string, not including the null-terminator,
 * will be written to size.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed, but without the NUL character, since
 * it cannot be represented in a null-terminated string.
 *
 * The allocated string must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 * if you want a null-terminator.
 *
 * @throws mpack_error_too_big If the string plus null-terminator is larger
 *     than the given maxsize.
 * @throws mpack_error_type If the value is not a string or contains
 *     invalid UTF-8 or a null byte.
 */
char* mpack_expect_utf8_cstr_alloc(mpack_reader_t* reader, size_t maxsize);
#endif

/**
 * Reads a string, ensuring it exactly matches the given null-terminated
 * string.
 *
 * Remember that maps are unordered in JSON. Don't use this for map keys
 * unless the map has only a single key!
 */
MPACK_INLINE void mpack_expect_cstr_match(mpack_reader_t* reader, const char* str) {
    if (mpack_strlen(str) > UINT32_MAX) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return;
    }
    mpack_expect_str_match(reader, str, mpack_strlen(str));
}

/**
 * @}
 */

/**
 * @name Binary Data / Extension Functions
 * @{
 */

/**
 * Reads the start of a binary blob, returning its size in bytes.
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_bin() must be called
 * once all bytes have been read.
 *
 * mpack_error_type is raised if the value is not a binary blob.
 */
uint32_t mpack_expect_bin(mpack_reader_t* reader);

/**
 * Reads the start of a binary blob, raising an error if its length is not
 * at most the given number of bytes.
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_bin() must be called
 * once all bytes have been read.
 *
 * mpack_error_type is raised if the value is not a binary blob or if its
 * length does not match.
 */
MPACK_INLINE uint32_t mpack_expect_bin_max(mpack_reader_t* reader, uint32_t maxsize) {
    uint32_t length = mpack_expect_bin(reader);
    if (length > maxsize) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return 0;
    }
    return length;
}

/**
 * Reads the start of a binary blob, raising an error if its length is not
 * exactly the given number of bytes.
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_bin() must be called
 * once all bytes have been read.
 *
 * mpack_error_type is raised if the value is not a binary blob or if its
 * length does not match.
 */
MPACK_INLINE void mpack_expect_bin_size(mpack_reader_t* reader, uint32_t count) {
    if (mpack_expect_bin(reader) != count)
        mpack_reader_flag_error(reader, mpack_error_type);
}

/**
 * Reads a binary blob into the given buffer, returning its size in bytes.
 *
 * For compatibility, this will accept if the underlying type is string or
 * binary (since in MessagePack 1.0, strings and binary data were combined
 * under the "raw" type which became string in 1.1.)
 */
size_t mpack_expect_bin_buf(mpack_reader_t* reader, char* buf, size_t size);

/**
 * Reads a binary blob with the given total maximum size, allocating storage for it.
 */
char* mpack_expect_bin_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size);

/**
 * @}
 */

/**
 * @name Special Functions
 * @{
 */

/**
 * Reads a MessagePack object header (an MPack tag), expecting it to exactly
 * match the given tag.
 *
 * If the type is compound (i.e. is a map, array, string, binary or
 * extension type), additional reads are required to get the contained
 * data, and the corresponding done function must be called when done.
 *
 * @throws mpack_error_type if the tag does not match
 *
 * @see mpack_read_bytes()
 * @see mpack_done_array()
 * @see mpack_done_map()
 * @see mpack_done_str()
 * @see mpack_done_bin()
 * @see mpack_done_ext()
 */
void mpack_expect_tag(mpack_reader_t* reader, mpack_tag_t tag);

/**
 * Expects a string matching one of the strings in the given array,
 * returning its array index.
 *
 * If the value does not match any of the given strings,
 * @ref mpack_error_type is flagged. Use mpack_expect_enum_optional()
 * if you want to allow other values than the given strings.
 *
 * If any error occurs or the reader is in an error state, @a count
 * is returned.
 *
 * This can be used to quickly parse a string into an enum when the
 * enum values range from 0 to @a count-1. If the last value in the
 * enum is a special "count" value, it can be passed as the count,
 * and the return value can be cast directly to the enum type.
 *
 * @code{.c}
 * typedef enum           { APPLE ,  BANANA ,  ORANGE , COUNT} fruit_t;
 * const char* fruits[] = {"apple", "banana", "orange"};
 *
 * fruit_t fruit = (fruit_t)mpack_expect_enum(reader, fruits, COUNT);
 * @endcode
 *
 * See @ref docs/expect.md for more examples.
 *
 * The maximum string length is the size of the buffer (strings are read in-place.)
 *
 * @param reader The reader
 * @param strings An array of expected strings of length count
 * @param count The number of strings
 * @return The index of the matched string, or @a count in case of error
 */
size_t mpack_expect_enum(mpack_reader_t* reader, const char* strings[], size_t count);

/**
 * Expects a string matching one of the strings in the given array
 * returning its array index, or @a count if no strings match.
 *
 * If the value is not a string, or it does not match any of the
 * given strings, @a count is returned and no error is flagged.
 *
 * If any error occurs or the reader is in an error state, @a count
 * is returned.
 *
 * This can be used to quickly parse a string into an enum when the
 * enum values range from 0 to @a count-1. If the last value in the
 * enum is a special "count" value, it can be passed as the count,
 * and the return value can be cast directly to the enum type.
 *
 * @code{.c}
 * typedef enum           { APPLE ,  BANANA ,  ORANGE , COUNT} fruit_t;
 * const char* fruits[] = {"apple", "banana", "orange"};
 *
 * fruit_t fruit = (fruit_t)mpack_expect_enum_optional(reader, fruits, COUNT);
 * @endcode
 *
 * See @ref docs/expect.md for more examples.
 *
 * The maximum string length is the size of the buffer (strings are read in-place.)
 *
 * @param reader The reader
 * @param strings An array of expected strings of length count
 * @param count The number of strings
 *
 * @return The index of the matched string, or @a count if it does not
 * match or an error occurs
 */
size_t mpack_expect_enum_optional(mpack_reader_t* reader, const char* strings[], size_t count);

/**
 * Expects an unsigned integer map key between 0 and count-1, marking it
 * as found in the given bool array and returning it.
 *
 * This is a helper for switching among int keys in a map. It is
 * typically used with an enum to define the key values. It should
 * be called in the expression of a switch() statement. See @ref
 * docs/expect.md for an example.
 *
 * The found array must be cleared before expecting the first key. If the
 * flag for a given key is already set when found (i.e. the map contains a
 * duplicate key), mpack_error_invalid is flagged.
 *
 * If the key is not a non-negative integer, or if the key is @a count or
 * larger, @a count is returned and no error is flagged. If you want an error
 * on unrecognized keys, flag an error in the default case in your switch;
 * otherwise you must call mpack_discard() to discard its content.
 *
 * @param reader The reader
 * @param found An array of bool flags of length count
 * @param count The number of values in the found array, and one more than the
 *              maximum allowed key
 *
 * @see @ref docs/expect.md
 */
size_t mpack_expect_key_uint(mpack_reader_t* reader, bool found[], size_t count);

/**
 * Expects a string map key matching one of the strings in the given key list,
 * marking it as found in the given bool array and returning its index.
 *
 * This is a helper for switching among string keys in a map. It is
 * typically used with an enum with names matching the strings in the
 * array to define the key indices. It should be called in the expression
 * of a switch() statement. See @ref docs/expect.md for an example.
 *
 * The found array must be cleared before expecting the first key. If the
 * flag for a given key is already set when found (i.e. the map contains a
 * duplicate key), mpack_error_invalid is flagged.
 *
 * If the key is unrecognized, count is returned and no error is flagged. If
 * you want an error on unrecognized keys, flag an error in the default case
 * in your switch; otherwise you must call mpack_discard() to discard its content.
 *
 * The maximum key length is the size of the buffer (keys are read in-place.)
 *
 * @param reader The reader
 * @param keys An array of expected string keys of length count
 * @param found An array of bool flags of length count
 * @param count The number of values in the keys and found arrays
 *
 * @see @ref docs/expect.md
 */
size_t mpack_expect_key_cstr(mpack_reader_t* reader, const char* keys[],
        bool found[], size_t count);

/**
 * @}
 */

/**
 * @}
 */

#endif

MPACK_HEADER_END

#endif


