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

MPACK_HEADER_START



/* Version information */

#define MPACK_VERSION_MAJOR 0  /**< The major version number of MPack. */
#define MPACK_VERSION_MINOR 7  /**< The minor version number of MPack. */
#define MPACK_VERSION_PATCH 0  /**< The patch version number of MPack. */

/** A number containing the version number of MPack for comparison purposes. */
#define MPACK_VERSION ((MPACK_VERSION_MAJOR * 10000) + \
        (MPACK_VERSION_MINOR * 100) + MPACK_VERSION_PATCH)

/** A macro to test for a minimum version of MPack. */
#define MPACK_VERSION_AT_LEAST(major, minor, patch) \
        (MPACK_VERSION >= (((major) * 10000) + ((minor) * 100) + (patch)))

/** @cond */
#if (MPACK_VERSION_PATCH > 0)
#define MPACK_VERSION_STRING_BASE \
        MPACK_STRINGIFY(MPACK_VERSION_MAJOR) "." \
        MPACK_STRINGIFY(MPACK_VERSION_MINOR) "." \
        MPACK_STRINGIFY(MPACK_VERSION_PATCH)
#else
#define MPACK_VERSION_STRING_BASE \
        MPACK_STRINGIFY(MPACK_VERSION_MAJOR) "." \
        MPACK_STRINGIFY(MPACK_VERSION_MINOR)
#endif
/** @endcond */

/**
 * @def MPACK_VERSION_STRING
 * @hideinitializer
 *
 * A string containing the MPack version.
 */
#if MPACK_AMALGAMATED
#define MPACK_VERSION_STRING MPACK_VERSION_STRING_BASE
#else
#define MPACK_VERSION_STRING MPACK_VERSION_STRING_BASE "dev"
#endif

/**
 * @def MPACK_LIBRARY_STRING
 * @hideinitializer
 *
 * A string describing MPack, containing the library name, version and debug mode.
 */
#if MPACK_DEBUG
#define MPACK_LIBRARY_STRING "MPack " MPACK_VERSION_STRING "-debug"
#else
#define MPACK_LIBRARY_STRING "MPack " MPACK_VERSION_STRING
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
 * Error states for MPack objects.
 *
 * When a reader, writer, or tree is in an error state, all subsequent calls
 * are ignored and their return values are nil/zero. You should check whether
 * the source is in an error state before using such values.
 */
typedef enum mpack_error_t {
    mpack_ok = 0,        /**< No error. */
    mpack_error_io = 2,  /**< The reader or writer failed to fill or flush, or some other file or socket error occurred. */
    mpack_error_invalid, /**< The data read is not valid MessagePack. */
    mpack_error_type,    /**< The type or value range did not match what was expected by the caller. */
    mpack_error_too_big, /**< A read or write was bigger than the maximum size allowed for that operation. */
    mpack_error_memory,  /**< An allocation failure occurred. */
    mpack_error_bug,     /**< The MPack API was used incorrectly. (This will always assert in debug mode.) */
    mpack_error_data,    /**< The contained data is not valid. */
} mpack_error_t;

/**
 * Converts an MPack error to a string. This function returns an empty
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
 * Converts an MPack type to a string. This function returns an empty
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
    union
    {
        bool     b; /**< The value if the type is bool. */
        float    f; /**< The value if the type is float. */
        double   d; /**< The value if the type is double. */
        int64_t  i; /**< The value if the type is signed int. */
        uint64_t u; /**< The value if the type is unsigned int. */
        uint32_t l; /**< The number of bytes if the type is str, bin or ext. */

        /** The element count if the type is an array, or the number of
            key/value pairs if the type is map. */
        uint32_t n;
    } v;
} mpack_tag_t;

/** Generates a nil tag. */
MPACK_INLINE mpack_tag_t mpack_tag_nil(void) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_nil;
    return ret;
}

/** Generates a bool tag. */
MPACK_INLINE mpack_tag_t mpack_tag_bool(bool value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bool;
    ret.v.b = value;
    return ret;
}

/** Generates a bool tag with value true. */
MPACK_INLINE mpack_tag_t mpack_tag_true(void) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bool;
    ret.v.b = true;
    return ret;
}

/** Generates a bool tag with value false. */
MPACK_INLINE mpack_tag_t mpack_tag_false(void) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bool;
    ret.v.b = false;
    return ret;
}

/** Generates a signed int tag. */
MPACK_INLINE mpack_tag_t mpack_tag_int(int64_t value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_int;
    ret.v.i = value;
    return ret;
}

/** Generates an unsigned int tag. */
MPACK_INLINE mpack_tag_t mpack_tag_uint(uint64_t value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_uint;
    ret.v.u = value;
    return ret;
}

/** Generates a float tag. */
MPACK_INLINE mpack_tag_t mpack_tag_float(float value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_float;
    ret.v.f = value;
    return ret;
}

/** Generates a double tag. */
MPACK_INLINE mpack_tag_t mpack_tag_double(double value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_double;
    ret.v.d = value;
    return ret;
}

/** Generates an array tag. */
MPACK_INLINE mpack_tag_t mpack_tag_array(int32_t count) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_array;
    ret.v.n = count;
    return ret;
}

/** Generates a map tag. */
MPACK_INLINE mpack_tag_t mpack_tag_map(int32_t count) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_map;
    ret.v.n = count;
    return ret;
}

/** Generates a str tag. */
MPACK_INLINE mpack_tag_t mpack_tag_str(int32_t length) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_str;
    ret.v.l = length;
    return ret;
}

/** Generates a bin tag. */
MPACK_INLINE mpack_tag_t mpack_tag_bin(int32_t length) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bin;
    ret.v.l = length;
    return ret;
}

/** Generates an ext tag. */
MPACK_INLINE mpack_tag_t mpack_tag_ext(int8_t exttype, int32_t length) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_ext;
    ret.exttype = exttype;
    ret.v.l = length;
    return ret;
}

/**
 * Compares two tags with an arbitrary fixed ordering. Returns 0 if the tags are
 * equal, a negative integer if left comes before right, or a positive integer
 * otherwise.
 *
 * \warning The ordering is not guaranteed to be preserved across MPack versions; do
 * not rely on it in persistent data.
 *
 * \warning Floating point numbers are compared bit-for-bit, not using the language's
 * operator==. This means that NaNs with matching representation will compare equal.
 * This behaviour is up for debate; see comments in the definition of mpack_tag_cmp().
 *
 * See mpack_tag_equal() for more information on when tags are considered equal.
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
 * \warning Floating point numbers are compared bit-for-bit, not using the language's
 * operator==. This means that NaNs with matching representation will compare equal.
 * This behaviour is up for debate; see comments in the definition of mpack_tag_cmp().
 */
MPACK_INLINE bool mpack_tag_equal(mpack_tag_t left, mpack_tag_t right) {
    return mpack_tag_cmp(left, right) == 0;
}

/**
 * @}
 */



/* Helpers for fetching an arbitrarily sized int from a memory
 * location, regardless of endianness or alignment. */
/** @cond */

MPACK_ALWAYS_INLINE uint8_t mpack_load_native_u8(const char* p) {
    return (uint8_t)p[0];
}

MPACK_ALWAYS_INLINE uint16_t mpack_load_native_u16(const char* p) {
    return (uint16_t)((((uint16_t)(uint8_t)p[0]) << 8) |
           ((uint16_t)(uint8_t)p[1]));
}

MPACK_ALWAYS_INLINE uint32_t mpack_load_native_u32(const char* p) {
    return (((uint32_t)(uint8_t)p[0]) << 24) |
           (((uint32_t)(uint8_t)p[1]) << 16) |
           (((uint32_t)(uint8_t)p[2]) <<  8) |
            ((uint32_t)(uint8_t)p[3]);
}

MPACK_ALWAYS_INLINE uint64_t mpack_load_native_u64(const char* p) {
    return (((uint64_t)(uint8_t)p[0]) << 56) |
           (((uint64_t)(uint8_t)p[1]) << 48) |
           (((uint64_t)(uint8_t)p[2]) << 40) |
           (((uint64_t)(uint8_t)p[3]) << 32) |
           (((uint64_t)(uint8_t)p[4]) << 24) |
           (((uint64_t)(uint8_t)p[5]) << 16) |
           (((uint64_t)(uint8_t)p[6]) <<  8) |
            ((uint64_t)(uint8_t)p[7]);
}

/** @endcond */



#if MPACK_READ_TRACKING || MPACK_WRITE_TRACKING
/* Tracks the write state of compound elements (maps, arrays, */
/* strings, binary blobs and extension types) */
/** @cond */

typedef struct mpack_track_element_t {
    mpack_type_t type;
    uint64_t left; // we need 64-bit because (2 * INT32_MAX) elements can be stored in a map
} mpack_track_element_t;

typedef struct mpack_track_t {
    size_t count;
    size_t capacity;
    mpack_track_element_t* elements;
} mpack_track_t;

#if MPACK_INTERNAL
mpack_error_t mpack_track_init(mpack_track_t* track);
mpack_error_t mpack_track_grow(mpack_track_t* track);
mpack_error_t mpack_track_push(mpack_track_t* track, mpack_type_t type, uint64_t count);
mpack_error_t mpack_track_pop(mpack_track_t* track, mpack_type_t type);
mpack_error_t mpack_track_element(mpack_track_t* track, bool read);
mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, uint64_t count);
mpack_error_t mpack_track_check_empty(mpack_track_t* track);
mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel);
#endif

/** @endcond */
#endif



#if MPACK_INTERNAL
/** @cond */



/* The below code is from Bjoern Hoehrmann's Flexible and Economical */
/* UTF-8 decoder, modified to support MPack inlining and add the mpack prefix. */

/* Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de> */
/* See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details. */

#define MPACK_UTF8_ACCEPT 0
#define MPACK_UTF8_REJECT 12

/**
 * Parses one byte from a UTF-8 stream.
 *
 * Returns and sets state to:
 *   - MPACK_UTF8_ACCEPT if the byte completes a valid unicode code point, placing it in codep
 *   - MPACK_UTF8_REJECT if the byte is invalid UTF-8
 *   - something else if more bytes are needed to form a valid character
 *
 * If more bytes are needed, this should be called again with the next byte
 * in the string. state and codep should not be modified, since they will
 * contain the partially read code point.
 *
 * The initial state should be set to MPACK_UTF8_ACCEPT before parsing a string.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Overlong sequences and UTF-16 surrogates will be rejected. Only
 * pure UTF-8 is accepted.
 */
uint32_t mpack_utf8_decode(uint32_t* state, uint32_t* codep, uint8_t byte);

/* End of UTF-8 decoder code */



/* Miscellaneous string functions */

/**
 * Returns true if the given UTF-8 string is valid.
 */
bool mpack_utf8_check(char* str, size_t bytes);

/**
 * Returns true if the given UTF-8 string is valid and contains no null characters.
 */
bool mpack_utf8_check_no_null(char* str, size_t bytes);

/**
 * Returns true if the given string has no null bytes.
 */
bool mpack_str_check_no_null(char* str, size_t bytes);



/** @endcond */
#endif



MPACK_HEADER_END

#endif

