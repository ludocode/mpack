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
 * Defines types and functions shared by the MPack reader and writer.
 */

#ifndef MPACK_COMMON_H
#define MPACK_COMMON_H 1

#include "mpack-platform.h"

MPACK_HEADER_START



/**
 * @defgroup common Common Elements
 *
 * Contains types, constants and functions shared by both the encoding
 * and decoding portions of MPack.
 *
 * @{
 */

/* Version information */

#define MPACK_VERSION_MAJOR 0  /**< The major version number of MPack. */
#define MPACK_VERSION_MINOR 9  /**< The minor version number of MPack. */
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
#if MPACK_RELEASE_VERSION
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

/** @cond */
/**
 * @def MPACK_MAXIMUM_TAG_SIZE
 *
 * The maximum encoded size of a tag in bytes, as of the "new" MessagePack spec.
 */
#define MPACK_MAXIMUM_TAG_SIZE 9
/** @endcond */



#if MPACK_COMPATIBILITY
/**
 * Versions of the MessagePack format.
 *
 * A reader, writer, or tree can be configured to serialize in an older
 * version of the MessagePack spec. This is necessary to interface with
 * older MessagePack libraries that do not support new MessagePack features.
 *
 * This requires @ref MPACK_COMPATIBILITY.
 */
typedef enum mpack_version_t {

    /**
     * Version 1.0/v4, supporting only the @c raw type without @c str8.
     */
    mpack_version_v4 = 4,

    /**
     * Version 2.0/v5, supporting the @c str8, @c bin and @c ext types.
     */
    mpack_version_v5 = 5,

    /**
     * The most recent supported version of MessagePack. This is the default.
     */
    mpack_version_current = mpack_version_v5,

} mpack_version_t;
#endif

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
    mpack_error_eof,     /**< The reader failed to read because of file or socket EOF */
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
    mpack_type_int,     /**< A 64-bit signed integer. */
    mpack_type_uint,    /**< A 64-bit unsigned integer. */
    mpack_type_float,   /**< A 32-bit IEEE 754 floating point number. */
    mpack_type_double,  /**< A 64-bit IEEE 754 floating point number. */
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
 *
 * This structure is opaque; its fields should not be accessed outside
 * of MPack.
 */
typedef struct mpack_tag_t mpack_tag_t;

/* Hide internals from documentation */
/** @cond */
struct mpack_tag_t {
    /* The value for non-compound types. */
    union
    {
        uint64_t u; /*< The value if the type is unsigned int. */
        int64_t  i; /*< The value if the type is signed int. */
        double   d; /*< The value if the type is double. */
        float    f; /*< The value if the type is float. */
        bool     b; /*< The value if the type is bool. */

        /* The number of bytes if the type is a str or bin. */
        uint32_t l;

        /* The element count if the type is an array, or the number of
            key/value pairs if the type is map. */
        uint32_t n;

        /*
         * The extension data if the type is @ref mpack_type_ext.
         */
        struct {
            int8_t exttype; /*< The extension type. */
            uint32_t length; /*< The number of bytes. */
        } ext;
    } v;

    /* The type of value. */
    mpack_type_t type;
};
/** @endcond */

/**
 * @name Tag Generators
 * @{
 */

/**
 * An @ref mpack_tag_t initializer that zeroes the given tag.
 *
 * This does not make the tag nil! The tag's type is invalid when initialized this way.
 */
#define MPACK_TAG_ZERO {{0}, (mpack_type_t)0}

/** Generates a nil tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_nil(void) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_nil;
    return ret;
}

/** Generates a bool tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_bool(bool value) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_bool;
    ret.v.b = value;
    return ret;
}

/** Generates a bool tag with value true. */
MPACK_INLINE mpack_tag_t mpack_tag_make_true(void) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_bool;
    ret.v.b = true;
    return ret;
}

/** Generates a bool tag with value false. */
MPACK_INLINE mpack_tag_t mpack_tag_make_false(void) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_bool;
    ret.v.b = false;
    return ret;
}

/** Generates a signed int tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_int(int64_t value) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_int;
    ret.v.i = value;
    return ret;
}

/** Generates an unsigned int tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_uint(uint64_t value) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_uint;
    ret.v.u = value;
    return ret;
}

/** Generates a float tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_float(float value) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_float;
    ret.v.f = value;
    return ret;
}

/** Generates a double tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_double(double value) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_double;
    ret.v.d = value;
    return ret;
}

/** Generates an array tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_array(uint32_t count) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_array;
    ret.v.n = count;
    return ret;
}

/** Generates a map tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_map(uint32_t count) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_map;
    ret.v.n = count;
    return ret;
}

/** Generates a str tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_str(uint32_t length) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_str;
    ret.v.l = length;
    return ret;
}

/** Generates a bin tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_bin(uint32_t length) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_bin;
    ret.v.l = length;
    return ret;
}

/** Generates an ext tag. */
MPACK_INLINE mpack_tag_t mpack_tag_make_ext(int8_t exttype, uint32_t length) {
    mpack_tag_t ret = MPACK_TAG_ZERO;
    ret.type = mpack_type_ext;
    ret.v.ext.exttype = exttype;
    ret.v.ext.length = length;
    return ret;
}

/**
 * @}
 */

/**
 * @name Tag Querying Functions
 * @{
 */

/**
 * Gets the type of a tag.
 */
MPACK_INLINE mpack_type_t mpack_tag_type(mpack_tag_t* tag) {
    return tag->type;
}

/**
 * Gets the boolean value of a bool-type tag. The tag must be of type @ref
 * mpack_type_bool.
 */
MPACK_INLINE bool mpack_tag_bool_value(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_bool, "tag is not a bool!");
    return tag->v.b;
}

/**
 * Gets the signed integer value of an int-type tag.
 *
 * This does not convert between signed and unsigned tags! A positive integer may be stored in a tag as either @ref mpack_type_int or @ref mpack_type_uint. You must check the type first; this can only be used if the type is @ref mpack_type_int.
 *
 * @see mpack_type_int
 */
MPACK_INLINE int64_t mpack_tag_int_value(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_int, "tag is not an int!");
    return tag->v.i;
}

/**
 * Gets the unsigned integer value of an uint-type tag.
 *
 * This does not convert between signed and unsigned tags! A positive integer may be stored in a tag as either @ref mpack_type_int or @ref mpack_type_uint. You must check the type first; this can only be used if the type is @ref mpack_type_uint.
 *
 * @see mpack_type_uint
 */
MPACK_INLINE uint64_t mpack_tag_uint_value(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_uint, "tag is not a uint!");
    return tag->v.u;
}

/**
 * Gets the float value of a float-type tag.
 *
 * This does not convert between float and double tags. This can only be used if the type is @ref mpack_type_float.
 *
 * @see mpack_type_float
 */
MPACK_INLINE float mpack_tag_float_value(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_float, "tag is not a float!");
    return tag->v.f;
}

/**
 * Gets the double value of a double-type tag.
 *
 * This does not convert between float and double tags. This can only be used if the type is @ref mpack_type_double.
 *
 * @see mpack_type_double
 */
MPACK_INLINE double mpack_tag_double_value(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_double, "tag is not a double!");
    return tag->v.d;
}

/**
 * Gets the number of elements in an array tag.
 *
 * @see mpack_type_array
 */
MPACK_INLINE uint32_t mpack_tag_array_count(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_array, "tag is not an array!");
    return tag->v.n;
}

/**
 * Gets the number of key-value pairs in a map tag.
 *
 * @see mpack_type_map
 */
MPACK_INLINE uint32_t mpack_tag_map_count(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_map, "tag is not a map!");
    return tag->v.n;
}

/**
 * Gets the length in bytes of a str-type tag.
 *
 * @see mpack_type_str
 */
MPACK_INLINE uint32_t mpack_tag_str_length(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_str, "tag is not a str!");
    return tag->v.l;
}

/**
 * Gets the length in bytes of a bin-type tag.
 *
 * @see mpack_type_bin
 */
MPACK_INLINE uint32_t mpack_tag_bin_length(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_bin, "tag is not a bin!");
    return tag->v.l;
}

/**
 * Gets the length in bytes of an ext-type tag.
 *
 * @see mpack_type_ext
 */
MPACK_INLINE uint32_t mpack_tag_ext_length(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_ext, "tag is not an ext!");
    return tag->v.ext.length;
}

/**
 * Gets the extension type (exttype) of an ext-type tag.
 *
 * @see mpack_type_ext
 */
MPACK_INLINE int8_t mpack_tag_ext_exttype(mpack_tag_t* tag) {
    mpack_assert(tag->type == mpack_type_ext, "tag is not an ext!");
    return tag->v.ext.exttype;
}

/**
 * @}
 */

/**
 * @name other tag functions
 * @{
 */

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

#if MPACK_DEBUG && MPACK_STDIO
/**
 * Generates a json-like debug description of the given tag into the given buffer.
 *
 * This is only available in debug mode, and only if stdio is available (since
 * it uses snprintf().) It's strictly for debugging purposes.
 */
void mpack_tag_debug_pseudo_json(mpack_tag_t tag, char* buffer, size_t buffer_size);

/**
 * Generates a debug string description of the given tag into the given buffer.
 *
 * This is only available in debug mode, and only if stdio is available (since
 * it uses snprintf().) It's strictly for debugging purposes.
 */
void mpack_tag_debug_describe(mpack_tag_t tag, char* buffer, size_t buffer_size);
#endif

/**
 * @}
 */

/**
 * @name Deprecated Tag Generators
 * @{
 */

/*
 * "make" has been added to their names to disambiguate them from the
 * value-fetching functions (e.g. mpack_tag_make_bool() vs
 * mpack_tag_bool_value().)
 *
 * The length and count for all compound types was the wrong sign (int32_t
 * instead of uint32_t.) These preserve the old behaviour; the new "make"
 * functions have the correct sign.
 */

/** \deprecated Renamed to mpack_tag_make_nil(). */
MPACK_INLINE mpack_tag_t mpack_tag_nil(void) {
    return mpack_tag_make_nil();
}

/** \deprecated Renamed to mpack_tag_make_bool(). */
MPACK_INLINE mpack_tag_t mpack_tag_bool(bool value) {
    return mpack_tag_make_bool(value);
}

/** \deprecated Renamed to mpack_tag_make_true(). */
MPACK_INLINE mpack_tag_t mpack_tag_true(void) {
    return mpack_tag_make_true();
}

/** \deprecated Renamed to mpack_tag_make_false(). */
MPACK_INLINE mpack_tag_t mpack_tag_false(void) {
    return mpack_tag_make_false();
}

/** \deprecated Renamed to mpack_tag_make_int(). */
MPACK_INLINE mpack_tag_t mpack_tag_int(int64_t value) {
    return mpack_tag_make_int(value);
}

/** \deprecated Renamed to mpack_tag_make_uint(). */
MPACK_INLINE mpack_tag_t mpack_tag_uint(uint64_t value) {
    return mpack_tag_make_uint(value);
}

/** \deprecated Renamed to mpack_tag_make_float(). */
MPACK_INLINE mpack_tag_t mpack_tag_float(float value) {
    return mpack_tag_make_float(value);
}

/** \deprecated Renamed to mpack_tag_make_double(). */
MPACK_INLINE mpack_tag_t mpack_tag_double(double value) {
    return mpack_tag_make_double(value);
}

/** \deprecated Renamed to mpack_tag_make_array(). */
MPACK_INLINE mpack_tag_t mpack_tag_array(int32_t count) {
    return mpack_tag_make_array((uint32_t)count);
}

/** \deprecated Renamed to mpack_tag_make_map(). */
MPACK_INLINE mpack_tag_t mpack_tag_map(int32_t count) {
    return mpack_tag_make_map((uint32_t)count);
}

/** \deprecated Renamed to mpack_tag_make_str(). */
MPACK_INLINE mpack_tag_t mpack_tag_str(int32_t length) {
    return mpack_tag_make_str((uint32_t)length);
}

/** \deprecated Renamed to mpack_tag_make_bin(). */
MPACK_INLINE mpack_tag_t mpack_tag_bin(int32_t length) {
    return mpack_tag_make_bin((uint32_t)length);
}

/** \deprecated Renamed to mpack_tag_make_ext(). */
MPACK_INLINE mpack_tag_t mpack_tag_ext(int8_t exttype, int32_t length) {
    return mpack_tag_make_ext(exttype, (uint32_t)length);
}

/**
 * @}
 */

/** @cond */

/*
 * Helpers to perform unaligned network-endian loads and stores
 * at arbitrary addresses. Byte-swapping builtins are used if they
 * are available and if they improve performance.
 *
 * These will remain available in the public API so feel free to
 * use them for other purposes, but they are undocumented.
 */

MPACK_INLINE uint8_t mpack_load_u8(const char* p) {
    return (uint8_t)p[0];
}

MPACK_INLINE uint16_t mpack_load_u16(const char* p) {
    #ifdef MPACK_NHSWAP16
    uint16_t val;
    mpack_memcpy(&val, p, sizeof(val));
    return MPACK_NHSWAP16(val);
    #else
    return (uint16_t)((((uint16_t)(uint8_t)p[0]) << 8) |
           ((uint16_t)(uint8_t)p[1]));
    #endif
}

MPACK_INLINE uint32_t mpack_load_u32(const char* p) {
    #ifdef MPACK_NHSWAP32
    uint32_t val;
    mpack_memcpy(&val, p, sizeof(val));
    return MPACK_NHSWAP32(val);
    #else
    return (((uint32_t)(uint8_t)p[0]) << 24) |
           (((uint32_t)(uint8_t)p[1]) << 16) |
           (((uint32_t)(uint8_t)p[2]) <<  8) |
            ((uint32_t)(uint8_t)p[3]);
    #endif
}

MPACK_INLINE uint64_t mpack_load_u64(const char* p) {
    #ifdef MPACK_NHSWAP64
    uint64_t val;
    mpack_memcpy(&val, p, sizeof(val));
    return MPACK_NHSWAP64(val);
    #else
    return (((uint64_t)(uint8_t)p[0]) << 56) |
           (((uint64_t)(uint8_t)p[1]) << 48) |
           (((uint64_t)(uint8_t)p[2]) << 40) |
           (((uint64_t)(uint8_t)p[3]) << 32) |
           (((uint64_t)(uint8_t)p[4]) << 24) |
           (((uint64_t)(uint8_t)p[5]) << 16) |
           (((uint64_t)(uint8_t)p[6]) <<  8) |
            ((uint64_t)(uint8_t)p[7]);
    #endif
}

MPACK_INLINE void mpack_store_u8(char* p, uint8_t val) {
    uint8_t* u = (uint8_t*)p;
    u[0] = val;
}

MPACK_INLINE void mpack_store_u16(char* p, uint16_t val) {
    #ifdef MPACK_NHSWAP16
    val = MPACK_NHSWAP16(val);
    mpack_memcpy(p, &val, sizeof(val));
    #else
    uint8_t* u = (uint8_t*)p;
    u[0] = (uint8_t)((val >> 8) & 0xFF);
    u[1] = (uint8_t)( val       & 0xFF);
    #endif
}

MPACK_INLINE void mpack_store_u32(char* p, uint32_t val) {
    #ifdef MPACK_NHSWAP32
    val = MPACK_NHSWAP32(val);
    mpack_memcpy(p, &val, sizeof(val));
    #else
    uint8_t* u = (uint8_t*)p;
    u[0] = (uint8_t)((val >> 24) & 0xFF);
    u[1] = (uint8_t)((val >> 16) & 0xFF);
    u[2] = (uint8_t)((val >>  8) & 0xFF);
    u[3] = (uint8_t)( val        & 0xFF);
    #endif
}

MPACK_INLINE void mpack_store_u64(char* p, uint64_t val) {
    #ifdef MPACK_NHSWAP64
    val = MPACK_NHSWAP64(val);
    mpack_memcpy(p, &val, sizeof(val));
    #else
    uint8_t* u = (uint8_t*)p;
    u[0] = (uint8_t)((val >> 56) & 0xFF);
    u[1] = (uint8_t)((val >> 48) & 0xFF);
    u[2] = (uint8_t)((val >> 40) & 0xFF);
    u[3] = (uint8_t)((val >> 32) & 0xFF);
    u[4] = (uint8_t)((val >> 24) & 0xFF);
    u[5] = (uint8_t)((val >> 16) & 0xFF);
    u[6] = (uint8_t)((val >>  8) & 0xFF);
    u[7] = (uint8_t)( val        & 0xFF);
    #endif
}

MPACK_INLINE int8_t  mpack_load_i8 (const char* p) {return (int8_t) mpack_load_u8 (p);}
MPACK_INLINE int16_t mpack_load_i16(const char* p) {return (int16_t)mpack_load_u16(p);}
MPACK_INLINE int32_t mpack_load_i32(const char* p) {return (int32_t)mpack_load_u32(p);}
MPACK_INLINE int64_t mpack_load_i64(const char* p) {return (int64_t)mpack_load_u64(p);}
MPACK_INLINE void mpack_store_i8 (char* p, int8_t  val) {mpack_store_u8 (p, (uint8_t) val);}
MPACK_INLINE void mpack_store_i16(char* p, int16_t val) {mpack_store_u16(p, (uint16_t)val);}
MPACK_INLINE void mpack_store_i32(char* p, int32_t val) {mpack_store_u32(p, (uint32_t)val);}
MPACK_INLINE void mpack_store_i64(char* p, int64_t val) {mpack_store_u64(p, (uint64_t)val);}

MPACK_INLINE float mpack_load_float(const char* p) {
    MPACK_CHECK_FLOAT_ORDER();
    union {
        float f;
        uint32_t u;
    } v;
    v.u = mpack_load_u32(p);
    return v.f;
}

MPACK_INLINE double mpack_load_double(const char* p) {
    MPACK_CHECK_FLOAT_ORDER();
    union {
        double d;
        uint64_t u;
    } v;
    v.u = mpack_load_u64(p);
    return v.d;
}

MPACK_INLINE void mpack_store_float(char* p, float value) {
    MPACK_CHECK_FLOAT_ORDER();
    union {
        float f;
        uint32_t u;
    } v;
    v.f = value;
    mpack_store_u32(p, v.u);
}

MPACK_INLINE void mpack_store_double(char* p, double value) {
    MPACK_CHECK_FLOAT_ORDER();
    union {
        double d;
        uint64_t u;
    } v;
    v.d = value;
    mpack_store_u64(p, v.u);
}

/** @endcond */



/** @cond */

// Sizes in bytes for the various possible tags
#define MPACK_TAG_SIZE_FIXUINT  1
#define MPACK_TAG_SIZE_U8       2
#define MPACK_TAG_SIZE_U16      3
#define MPACK_TAG_SIZE_U32      5
#define MPACK_TAG_SIZE_U64      9
#define MPACK_TAG_SIZE_FIXINT   1
#define MPACK_TAG_SIZE_I8       2
#define MPACK_TAG_SIZE_I16      3
#define MPACK_TAG_SIZE_I32      5
#define MPACK_TAG_SIZE_I64      9
#define MPACK_TAG_SIZE_FLOAT    5
#define MPACK_TAG_SIZE_DOUBLE   9
#define MPACK_TAG_SIZE_FIXARRAY 1
#define MPACK_TAG_SIZE_ARRAY16  3
#define MPACK_TAG_SIZE_ARRAY32  5
#define MPACK_TAG_SIZE_FIXMAP   1
#define MPACK_TAG_SIZE_MAP16    3
#define MPACK_TAG_SIZE_MAP32    5
#define MPACK_TAG_SIZE_FIXSTR   1
#define MPACK_TAG_SIZE_STR8     2
#define MPACK_TAG_SIZE_STR16    3
#define MPACK_TAG_SIZE_STR32    5
#define MPACK_TAG_SIZE_BIN8     2
#define MPACK_TAG_SIZE_BIN16    3
#define MPACK_TAG_SIZE_BIN32    5
#define MPACK_TAG_SIZE_FIXEXT1  2
#define MPACK_TAG_SIZE_FIXEXT2  2
#define MPACK_TAG_SIZE_FIXEXT4  2
#define MPACK_TAG_SIZE_FIXEXT8  2
#define MPACK_TAG_SIZE_FIXEXT16 2
#define MPACK_TAG_SIZE_EXT8     3
#define MPACK_TAG_SIZE_EXT16    4
#define MPACK_TAG_SIZE_EXT32    6

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
mpack_error_t mpack_track_peek_element(mpack_track_t* track, bool read);
mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, uint64_t count);
mpack_error_t mpack_track_str_bytes_all(mpack_track_t* track, bool read, uint64_t count);
mpack_error_t mpack_track_check_empty(mpack_track_t* track);
mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel);
#endif

/** @endcond */
#endif



#if MPACK_INTERNAL
/** @cond */



/* Miscellaneous string functions */

/**
 * Returns true if the given UTF-8 string is valid.
 */
bool mpack_utf8_check(const char* str, size_t bytes);

/**
 * Returns true if the given UTF-8 string is valid and contains no null characters.
 */
bool mpack_utf8_check_no_null(const char* str, size_t bytes);

/**
 * Returns true if the given string has no null bytes.
 */
bool mpack_str_check_no_null(const char* str, size_t bytes);



/** @endcond */
#endif



MPACK_HEADER_END

#endif

