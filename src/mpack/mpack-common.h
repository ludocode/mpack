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
 * Defines types and functions shared by the MPack reader and writer.
 */

#ifndef MPACK_COMMON_H
#define MPACK_COMMON_H 1

#include "mpack-platform.h"

#ifndef MPACK_STACK_SIZE
#define MPACK_STACK_SIZE 4096
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup common Common Elements
 *
 * Contains types and functions shared by both the encoding and decoding
 * portions of MPack.
 *
 * @{
 */

/**
 * Error states for the MPack reader and writer.
 *
 * When a reader or writer is in an error state, all subsequent calls are
 * ignored and their return values are nil/zero. You should check whether
 * the reader or writer is in an error state before using such values.
 */
typedef enum mpack_error_t {
    mpack_ok = 0,        /**< No error. */
    mpack_error_io = 2,  /**< The reader or writer failed to fill or flush, or some other file or socket error occurred. */
    mpack_error_invalid, /**< The data read is not valid MessagePack. */
    mpack_error_type,    /**< The type or value range did not match what was expected by the caller. */
    mpack_error_too_big, /**< A read or write was bigger than the maximum size allowed for that operation. */
    mpack_error_memory,  /**< An allocation failure occurred. */
    mpack_error_bug,     /**< The API was used incorrectly. */
    mpack_error_data,    /**< The contained data is not valid. */
} mpack_error_t;

/**
 * Converts an mpack error to a string. This function returns an empty
 * string when MPACK_DEBUG is not set.
 */
const char* mpack_error_to_string(mpack_error_t error);

/**
 * Defines the type of a MessagePack tag.
 */
typedef enum mpack_type_t {
    mpack_type_nil = 1, /**< A null value. */
    mpack_type_bool,    /**< A boolean (true or false.) */
    mpack_type_float,   /**< A 32-bit IEEE 754 floating point number. */
    mpack_type_double,  /**< A 64-bit IEEE 754 floating point number. */
    mpack_type_int,     /**< A 64-bit signed integer. */
    mpack_type_uint,    /**< A 64-bit unsigned integer. */
    mpack_type_str,     /**< A string. */
    mpack_type_bin,     /**< A chunk of binary data. */
    mpack_type_ext,     /**< A typed MessagePack extension object containing a chunk of binary data. */
    mpack_type_array,   /**< An array of MessagePack objects. */
    mpack_type_map,     /**< An ordered map of key/value pairs of MessagePack objects. */
} mpack_type_t;

/**
 * Converts an mpack type to a string. This function returns an empty
 * string when MPACK_DEBUG is not set.
 */
const char* mpack_type_to_string(mpack_type_t type);

/**
 * An MPack tag is a MessagePack object header. It is a variant type representing
 * any kind of object, and includes the value of that object when it is not a
 * compound type (i.e. boolean, integer, float.)
 *
 * If the type is compound (str, bin, ext, array or map), the embedded data is
 * stored separately.
 */
typedef struct mpack_tag_t {
    mpack_type_t type; /**< The type of value. */

    int8_t exttype; /**< The extension type if the type is @ref mpack_type_ext. */

    /** The value for non-compound types. */
    union {
        bool     b; /**< The value if the type is bool. */
        float    f; /**< The value if the type is float. */
        double   d; /**< The value if the type is double. */
        int64_t  i; /**< The value if the type is signed int. */

        /**
         * The value if the type is unsigned int; the element count if
         * the type is array; the number of key/value pairs if the type
         * is map; or the number of bytes if the type is str, bin or ext.
         */
        uint64_t u;
    } v;
} mpack_tag_t;

/** Generates a nil tag. */
static inline mpack_tag_t mpack_tag_nil(void) {
    mpack_tag_t ret;
    ret.type = mpack_type_nil;
    return ret;
}

/** Generates a signed int tag. */
static inline mpack_tag_t mpack_tag_int(int64_t value) {
    mpack_tag_t ret;
    ret.type = mpack_type_int;
    ret.v.i = value;
    return ret;
}

/** Generates an unsigned int tag. */
static inline mpack_tag_t mpack_tag_uint(uint64_t value) {
    mpack_tag_t ret;
    ret.type = mpack_type_uint;
    ret.v.u = value;
    return ret;
}

/** Generates a bool tag. */
static inline mpack_tag_t mpack_tag_bool(bool value) {
    mpack_tag_t ret;
    ret.type = mpack_type_bool;
    ret.v.b = value;
    return ret;
}

/** Generates a float tag. */
static inline mpack_tag_t mpack_tag_float(float value) {
    mpack_tag_t ret;
    ret.type = mpack_type_float;
    ret.v.f = value;
    return ret;
}

/** Generates a double tag. */
static inline mpack_tag_t mpack_tag_double(double value) {
    mpack_tag_t ret;
    ret.type = mpack_type_double;
    ret.v.d = value;
    return ret;
}

/**
 * Compares two tags with an arbitrary fixed ordering. Returns 0 if the tags are
 * equal, a negative integer if left comes before right, or a positive integer
 * otherwise.
 *
 * See mpack_tag_equal() for information on when tags are considered
 * to be equal.
 *
 * The ordering is not guaranteed to be preserved across mpack versions; do not
 * rely on it in serialized data.
 */
int mpack_tag_cmp(mpack_tag_t left, mpack_tag_t right);

/**
 * Compares two tags for equality. Tags are considered equal if the types are compatible
 * and the values (for non-compound types) are equal.
 *
 * The field width of variable-width fields is ignored (and in fact is not stored
 * in a tag), and positive numbers in signed integers are considered equal to their
 * unsigned counterparts. So for example the value 1 stored as a positive fixint
 * is equal to the value 1 stored in a 64-bit unsigned integer field.
 *
 * The "extension type" of an extension object is considered part of the value
 * and much match exactly.
 *
 * Floating point numbers are compared bit-for-bit, not using the language's operator==.
 */
static inline bool mpack_tag_equal(mpack_tag_t left, mpack_tag_t right) {
    return mpack_tag_cmp(left, right) == 0;
}



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

