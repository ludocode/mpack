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

/** @cond */
#ifndef MPACK_STACK_SIZE
#define MPACK_STACK_SIZE 4096
#endif
/** @endcond */



/* Version information */

#define MPACK_VERSION_MAJOR 0  /**< The major version number of MPack. */
#define MPACK_VERSION_MINOR 5  /**< The minor version number of MPack. */
#define MPACK_VERSION_PATCH 2  /**< The patch version number of MPack. */

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
#ifdef MPACK_AMALGAMATED
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
#ifdef MPACK_DEBUG
#define MPACK_LIBRARY_STRING "MPack " MPACK_VERSION_STRING "-debug"
#else
#define MPACK_LIBRARY_STRING "MPack " MPACK_VERSION_STRING
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

/** Generates a bool tag. */
MPACK_INLINE mpack_tag_t mpack_tag_bool(bool value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bool;
    ret.v.b = value;
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



#if defined(MPACK_READ_TRACKING) || defined(MPACK_WRITE_TRACKING)

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

#ifdef MPACK_INTERNAL
mpack_error_t mpack_track_init(mpack_track_t* track);
mpack_error_t mpack_track_grow(mpack_track_t* track);

// These look like some overly large inline functions, but really
// they are mostly asserts. They boil down to just a few checks
// and assignments.

MPACK_INLINE_SPEED mpack_error_t mpack_track_push(mpack_track_t* track, mpack_type_t type, uint64_t count);
MPACK_INLINE_SPEED mpack_error_t mpack_track_pop(mpack_track_t* track, mpack_type_t type);
MPACK_INLINE_SPEED mpack_error_t mpack_track_element(mpack_track_t* track, bool read);
MPACK_INLINE_SPEED mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, uint64_t count);
MPACK_INLINE_SPEED mpack_error_t mpack_track_check_empty(mpack_track_t* track);
MPACK_INLINE_SPEED mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel);

#if MPACK_DEFINE_INLINE_SPEED
MPACK_INLINE_SPEED mpack_error_t mpack_track_push(mpack_track_t* track, mpack_type_t type, uint64_t count) {
    mpack_assert(track->elements, "null track elements!");

    // maps have twice the number of elements (key/value pairs)
    if (type == mpack_type_map)
        count *= 2;

    // grow if needed
    if (track->count == track->capacity) {
        mpack_error_t error = mpack_track_grow(track);
        if (error != mpack_ok)
            return error;
    }

    // insert new track
    track->elements[track->count].type = type;
    track->elements[track->count].left = count;
    ++track->count;
    return mpack_ok;
}

MPACK_INLINE_SPEED mpack_error_t mpack_track_pop(mpack_track_t* track, mpack_type_t type) {
    mpack_assert(track->elements, "null track elements!");

    if (track->count == 0) {
        mpack_break("attempting to close a %s but nothing was opened!", mpack_type_to_string(type));
        return mpack_error_bug;
    }

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type != type) {
        mpack_break("attempting to close a %s but the open element is a %s!",
                mpack_type_to_string(type), mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left != 0) {
        mpack_break("attempting to close a %s but there are %" PRIu64 " %s left",
                mpack_type_to_string(type), element->left,
                (type == mpack_type_map || type == mpack_type_array) ? "elements" : "bytes");
        return mpack_error_bug;
    }

    --track->count;
    return mpack_ok;
}

MPACK_INLINE_SPEED mpack_error_t mpack_track_element(mpack_track_t* track, bool read) {
    MPACK_UNUSED(read);
    mpack_assert(track->elements, "null track elements!");

    // if there are no open elements, that's fine, we can read elements at will
    if (track->count == 0)
        return mpack_ok;

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type != mpack_type_map && element->type != mpack_type_array) {
        mpack_break("elements cannot be %s within an %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left == 0) {
        mpack_break("too many elements %s for %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    --element->left;
    return mpack_ok;
}

MPACK_INLINE_SPEED mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, uint64_t count) {
    MPACK_UNUSED(read);
    mpack_assert(track->elements, "null track elements!");

    if (track->count == 0) {
        mpack_break("bytes cannot be %s with no open bin, str or ext", read ? "read" : "written");
        return mpack_error_bug;
    }

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type == mpack_type_map || element->type == mpack_type_array) {
        mpack_break("bytes cannot be %s within an %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left < count) {
        mpack_break("too many bytes %s for %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    element->left -= count;
    return mpack_ok;
}

MPACK_INLINE_SPEED mpack_error_t mpack_track_check_empty(mpack_track_t* track) {
    if (track->count != 0) {
        mpack_assert(0, "unclosed %s", mpack_type_to_string(track->elements[0].type));
        return mpack_error_bug;
    }
    return mpack_ok;
}

MPACK_INLINE_SPEED mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel) {
    mpack_error_t error = cancel ? mpack_ok : mpack_track_check_empty(track);
    MPACK_FREE(track->elements);
    track->elements = NULL;
    return error;
}
#endif

#endif
/** @endcond */
#endif



#ifdef MPACK_INTERNAL

/* The below code is from Bjoern Hoehrmann's Flexible and Economical */
/* UTF-8 decoder, modified to support MPack inlining and add the mpack prefix. */

/* Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de> */
/* See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details. */

#define MPACK_UTF8_ACCEPT 0
#define MPACK_UTF8_REJECT 12

MPACK_INLINE_SPEED uint32_t mpack_utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte);

#if MPACK_DEFINE_INLINE_SPEED
extern const uint8_t mpack_utf8d[];

MPACK_INLINE_SPEED uint32_t mpack_utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = mpack_utf8d[byte];

  *codep = (*state != MPACK_UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = mpack_utf8d[256 + *state + type];
  return *state;
}
#endif

#endif



#ifdef __cplusplus
}
#endif

#endif

