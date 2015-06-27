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
static inline mpack_tag_t mpack_tag_nil(void) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_nil;
    return ret;
}

/** Generates a signed int tag. */
static inline mpack_tag_t mpack_tag_int(int64_t value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_int;
    ret.v.i = value;
    return ret;
}

/** Generates an unsigned int tag. */
static inline mpack_tag_t mpack_tag_uint(uint64_t value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_uint;
    ret.v.u = value;
    return ret;
}

/** Generates a bool tag. */
static inline mpack_tag_t mpack_tag_bool(bool value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_bool;
    ret.v.b = value;
    return ret;
}

/** Generates a float tag. */
static inline mpack_tag_t mpack_tag_float(float value) {
    mpack_tag_t ret;
    mpack_memset(&ret, 0, sizeof(ret));
    ret.type = mpack_type_float;
    ret.v.f = value;
    return ret;
}

/** Generates a double tag. */
static inline mpack_tag_t mpack_tag_double(double value) {
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
static inline bool mpack_tag_equal(mpack_tag_t left, mpack_tag_t right) {
    return mpack_tag_cmp(left, right) == 0;
}

/**
 * A teardown function to be called when an MPack object is destroyed.
 */
typedef void (*mpack_teardown_t)(void* context);

/**
 * @}
 */



#if MPACK_READ_TRACKING || MPACK_WRITE_TRACKING

/* Tracks the write state of compound elements (maps, arrays, */
/* strings, binary blobs and extension types) */

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
mpack_error_t mpack_track_push(mpack_track_t* track, mpack_type_t type, uint64_t count);
mpack_error_t mpack_track_pop(mpack_track_t* track, mpack_type_t type);
mpack_error_t mpack_track_element(mpack_track_t* track, bool read);
mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, uint64_t count);

static inline mpack_error_t mpack_track_check_empty(mpack_track_t* track) {
    if (track->count != 0) {
        mpack_assert(0, "unclosed %s", mpack_type_to_string(track->elements[0].type));
        return mpack_error_bug;
    }
    return mpack_ok;
}

static inline mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel) {
    mpack_error_t error = mpack_track_check_empty(track);
    MPACK_FREE(track->elements);
    track->elements = NULL;
    return cancel ? mpack_ok : error;
}
#endif

#endif



#if MPACK_INTERNAL

/* The below code is from Bjoern Hoehrmann's Flexible and Economical */
/* UTF-8 decoder, modified to make it static and add the mpack prefix. */

/* Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de> */
/* See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details. */

#define MPACK_UTF8_ACCEPT 0
#define MPACK_UTF8_REJECT 12

static const uint8_t mpack_utf8d[] = {
  /* The first part of the table maps bytes to character classes that */
  /* to reduce the size of the transition table and create bitmasks. */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  /* The second part is a transition table that maps a combination */
  /* of a state of the automaton and a character class to a state. */
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};

static inline
uint32_t mpack_utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = mpack_utf8d[byte];

  *codep = (*state != MPACK_UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = mpack_utf8d[256 + *state + type];
  return *state;
}

#endif



#ifdef __cplusplus
}
#endif

#endif

