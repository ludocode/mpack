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

/**
 * @file
 *
 * Declares the MPack static Expect API.
 */

#ifndef MPACK_EXPECT_H
#define MPACK_EXPECT_H 1

#include "mpack-reader.h"

#if MPACK_EXPECT

#if !MPACK_READER
#error MPACK_EXPECT requires MPACK_READER.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup expect Expect API
 *
 * The MPack Expect API allows you to easily read MessagePack data when you
 * expect it to follow a predefined schema.
 *
 * The main purpose of the Expect API is convenience, so the API is lax. It
 * allows overlong / inefficiently encoded sequences, and it automatically
 * converts between similar types where there is no loss of precision (unless
 * otherwise noted.) It will convert from unsigned to signed or from float to
 * double for example.
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
 * Note that reading a double or a large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
float mpack_expect_float(mpack_reader_t* reader);

/**
 * Reads a number, returning the value as a double. The underlying value can be an
 * integer, float or double; the value is converted to a double.
 *
 * Note that reading a very large integer with this function can incur a
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

static inline uint8_t mpack_expect_u8_max(mpack_reader_t* reader, uint8_t max_value) {
    return mpack_expect_u8_range(reader, 0, max_value);
}

static inline uint16_t mpack_expect_u16_max(mpack_reader_t* reader, uint16_t max_value) {
    return mpack_expect_u16_range(reader, 0, max_value);
}

static inline uint32_t mpack_expect_u32_max(mpack_reader_t* reader, uint32_t max_value) {
    return mpack_expect_u32_range(reader, 0, max_value);
}

static inline uint64_t mpack_expect_u64_max(mpack_reader_t* reader, uint64_t max_value) {
    return mpack_expect_u64_range(reader, 0, max_value);
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
 * Reads a number, ensuring that it falls within the given range and returning
 * the value as a float. The underlying value can be an integer, float or
 * double; the value is converted to a float.
 *
 * Note that reading a double or a large integer with this function can incur a
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
 * Note that reading a very large integer with this function can incur a
 * loss of precision.
 *
 * @throws mpack_error_type if the underlying value is not a float, double or integer.
 */
double mpack_expect_double_range(mpack_reader_t* reader, double min_value, double max_value);

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
 * Reads a nil.
 */
void mpack_expect_nil(mpack_reader_t* reader);

/**
 * Reads a bool. Note that integers will raise mpack_error_type; the value
 * must be strictly a bool.
 */
bool mpack_expect_bool(mpack_reader_t* reader);

/**
 * Reads a bool, raising a mpack_error_type if it is not true.
 */
void mpack_expect_true(mpack_reader_t* reader);

/**
 * Reads a bool, raising a mpack_error_type if it is not false.
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
 * mpack_error_type is raised if the value is not a map.
 *
 * Note that maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
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
 * mpack_error_type is raised if the value is not a map or if its size does
 * not fall within the given range.
 *
 * Note that maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 */
uint32_t mpack_expect_map_range(mpack_reader_t* reader, uint32_t min_count, uint32_t max_count);

/**
 * Reads the start of a map of the exact size given.
 *
 * A number of values follow equal to twice the element count of the map,
 * alternating between keys and values. @ref mpack_done_map() must be called
 * once all elements have been read.
 *
 * @ref mpack_error_type is raised if the value is not a map or if its size
 * does not match the given count.
 *
 * Note that maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 */
void mpack_expect_map_match(mpack_reader_t* reader, uint32_t count);

bool mpack_expect_map_or_nil(mpack_reader_t* reader, uint32_t* count);

/**
 * Reads the start of an array, returning its element count.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 */
uint32_t mpack_expect_array(mpack_reader_t* reader);

/**
 * Reads the start of an array with a number of elements in the given range,
 * returning its element count.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * @throws mpack_error_type if the value is not an array or if its size does
 * not fall within the given range.
 */
uint32_t mpack_expect_array_range(mpack_reader_t* reader, uint32_t min_count, uint32_t max_count);

static inline uint32_t mpack_expect_array_max(mpack_reader_t* reader, uint32_t max_count) {
    return mpack_expect_array_range(reader, 0, max_count);
}

/**
 * Reads the start of an array of the exact size given.
 *
 * A number of values follow equal to the element count of the array.
 * @ref mpack_done_array() must be called once all elements have been read.
 *
 * @throws mpack_error_type if the value is not an array or if its size does
 * not fall within the given range.
 */
void mpack_expect_array_match(mpack_reader_t* reader, uint32_t count);

#ifdef MPACK_MALLOC
/**
 * @hideinitializer
 *
 * Reads the start of an array and allocates storage for it, placing its
 * size in count. A number of objects follow equal to the element count
 * of the array.
 */
#define mpack_expect_array_alloc(reader, Type, max_count, count) \
    ((Type*)mpack_expect_array_alloc_impl(reader, sizeof(Type), max_count, count))
#endif

/**
 * @}
 */

#ifdef MPACK_MALLOC
void* mpack_expect_array_alloc_impl(mpack_reader_t* reader,
        size_t element_size, uint32_t max_count, size_t* count);
#endif


/**
 * @name String Functions
 * @{
 */

/**
 * Reads the start of a string, returning its size in bytes.
 *
 * The bytes follow and must be read separately with mpack_read_bytes()
 * or mpack_read_bytes_inplace(). @ref mpack_done_str() must be called
 * once all bytes have been read.
 *
 * mpack_error_type is raised if the value is not a string.
 */
uint32_t mpack_expect_str(mpack_reader_t* reader);

void mpack_expect_str_length(mpack_reader_t* reader, uint32_t count);

/**
 * Reads a string with the given total maximum size, allocating storage for it.
 * A null-terminator will be added to the string. The length in bytes of the string,
 * not including the null-terminator, will be written to size.
 */
char* mpack_expect_str_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size);

/**
 * Reads a string with the given total maximum size, allocating storage for it
 * and ensuring it is valid UTF-8. A null-terminator will be added to the string.
 * The length in bytes of the string, not including the null-terminator,
 * will be written to size.
 */
char* mpack_expect_utf8_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size);

/**
 * Reads a string into the given buffer, ensures it has no null-bytes,
 * and adds null-terminator at the end.
 *
 * Raises mpack_error_too_big if there is not enough room for the string and null-terminator.
 * Raises mpack_error_type if the value is not a string or contains a null byte.
 */
void mpack_expect_cstr(mpack_reader_t* reader, char* buf, size_t size);

/**
 * Reads a string into the given buffer, ensures it is a valid UTF-8 string,
 * and adds null-terminator at the end.
 *
 * Raises mpack_error_too_big if there is not enough room for the string and null-terminator.
 * Raises mpack_error_type if the value is not a string or is not a valid UTF-8 string.
 */
void mpack_expect_utf8_cstr(mpack_reader_t* reader, char* buf, size_t size);

/**
 * Reads a string, allocates storage for it, ensures it has no null-bytes,
 * and adds null-terminator at the end. You assume ownership of the
 * returned pointer if reading succeeds.
 *
 * Raises mpack_error_too_big if the string plus null-terminator is larger than the given maxsize.
 * Raises mpack_error_invalid if the value is not a string or contains a null byte.
 */
char* mpack_expect_cstr_alloc(mpack_reader_t* reader, size_t maxsize);

/**
 * Reads a string, ensuring it exactly matches the given null-terminated
 * string.
 *
 * Remember that maps are unordered in JSON. Don't use this for map keys
 * unless the map has only a single key!
 */
void mpack_expect_cstr_match(mpack_reader_t* reader, const char* str);

/**
 * @}
 */

/**
 * @name Binary Data / Extension Functions
 * @{
 */

/**
 * Reads a binary blob into the given buffer, returning its size in bytes.
 *
 * For compatibility, this will accept if the underlying type is string or
 * binary (since in MessagePack 1.0, strings and binary data were combined
 * under the "raw" type which became string in 1.1.)
 */
size_t mpack_expect_bin_buf(mpack_reader_t* reader, char* buf, size_t size);

const char* mpack_expect_bin_inplace(mpack_reader_t* reader, size_t maxsize, size_t* size);

/**
 * Reads a binary blob with the given total maximum size, allocating storage for it.
 */
char* mpack_expect_bin_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size);

/**
 * Reads an extension object with the given total maximum size, allocating storage
 * for it. The extension type will be written to exttype, and the size will be
 * written to size.
 */
char* mpack_expect_ext_alloc(mpack_reader_t* reader, size_t maxsize, uint8_t* exttype, size_t* size);

/**
 * Reads an extension object of the given type with the given total maximum size,
 * allocating storage for it. The size will be written to size.
 */
char* mpack_expect_ext_type_alloc(mpack_reader_t* reader, uint8_t exttype, size_t maxsize, size_t* size);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
#endif


