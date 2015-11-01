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

#define MPACK_INTERNAL 1

#include "mpack-expect.h"

#if MPACK_EXPECT


// Basic Number Functions

uint8_t mpack_expect_u8(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= UINT8_MAX)
            return (uint8_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= 0 && var.v.i <= UINT8_MAX)
            return (uint8_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

uint16_t mpack_expect_u16(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= UINT16_MAX)
            return (uint16_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= 0 && var.v.i <= UINT16_MAX)
            return (uint16_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

uint32_t mpack_expect_u32(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= UINT32_MAX)
            return (uint32_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= 0 && var.v.i <= UINT32_MAX)
            return (uint32_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

uint64_t mpack_expect_u64(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        return var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= 0)
            return (uint64_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

int8_t mpack_expect_i8(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= INT8_MAX)
            return (int8_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= INT8_MIN && var.v.i <= INT8_MAX)
            return (int8_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

int16_t mpack_expect_i16(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= INT16_MAX)
            return (int16_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= INT16_MIN && var.v.i <= INT16_MAX)
            return (int16_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

int32_t mpack_expect_i32(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= INT32_MAX)
            return (int32_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        if (var.v.i >= INT32_MIN && var.v.i <= INT32_MAX)
            return (int32_t)var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

int64_t mpack_expect_i64(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint) {
        if (var.v.u <= INT64_MAX)
            return (int64_t)var.v.u;
    } else if (var.type == mpack_type_int) {
        return var.v.i;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

float mpack_expect_float(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint)
        return (float)var.v.u;
    else if (var.type == mpack_type_int)
        return (float)var.v.i;
    else if (var.type == mpack_type_float)
        return var.v.f;
    else if (var.type == mpack_type_double)
        return (float)var.v.d;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0.0f;
}

double mpack_expect_double(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_uint)
        return (double)var.v.u;
    else if (var.type == mpack_type_int)
        return (double)var.v.i;
    else if (var.type == mpack_type_float)
        return (double)var.v.f;
    else if (var.type == mpack_type_double)
        return var.v.d;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0.0;
}

float mpack_expect_float_strict(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_float)
        return var.v.f;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0.0f;
}

double mpack_expect_double_strict(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_float)
        return (double)var.v.f;
    else if (var.type == mpack_type_double)
        return var.v.d;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0.0;
}


// Ranged Number Functions
//
// All ranged functions are identical other than the type, so we
// define their content with a macro. The prototypes are still written
// out in full to support ctags/IDE tools.

#define MPACK_EXPECT_RANGE_IMPL(name, type_t)                           \
                                                                        \
    /* make sure the range is sensible */                               \
    mpack_assert(min_value <= max_value,                                \
            "min_value %i must be less than or equal to max_value %i",  \
            min_value, max_value);                                      \
                                                                        \
    /* read the value */                                                \
    type_t val = mpack_expect_##name(reader);                           \
    if (mpack_reader_error(reader) != mpack_ok)                         \
        return min_value;                                               \
                                                                        \
    /* make sure it fits */                                             \
    if (val < min_value || val > max_value) {                           \
        mpack_reader_flag_error(reader, mpack_error_type);              \
        return min_value;                                               \
    }                                                                   \
                                                                        \
    return val;

uint8_t mpack_expect_u8_range(mpack_reader_t* reader, uint8_t min_value, uint8_t max_value) {MPACK_EXPECT_RANGE_IMPL(u8, uint8_t)}
uint16_t mpack_expect_u16_range(mpack_reader_t* reader, uint16_t min_value, uint16_t max_value) {MPACK_EXPECT_RANGE_IMPL(u16, uint16_t)}
uint32_t mpack_expect_u32_range(mpack_reader_t* reader, uint32_t min_value, uint32_t max_value) {MPACK_EXPECT_RANGE_IMPL(u32, uint32_t)}
uint64_t mpack_expect_u64_range(mpack_reader_t* reader, uint64_t min_value, uint64_t max_value) {MPACK_EXPECT_RANGE_IMPL(u64, uint64_t)}

int8_t mpack_expect_i8_range(mpack_reader_t* reader, int8_t min_value, int8_t max_value) {MPACK_EXPECT_RANGE_IMPL(i8, int8_t)}
int16_t mpack_expect_i16_range(mpack_reader_t* reader, int16_t min_value, int16_t max_value) {MPACK_EXPECT_RANGE_IMPL(i16, int16_t)}
int32_t mpack_expect_i32_range(mpack_reader_t* reader, int32_t min_value, int32_t max_value) {MPACK_EXPECT_RANGE_IMPL(i32, int32_t)}
int64_t mpack_expect_i64_range(mpack_reader_t* reader, int64_t min_value, int64_t max_value) {MPACK_EXPECT_RANGE_IMPL(i64, int64_t)}

float mpack_expect_float_range(mpack_reader_t* reader, float min_value, float max_value) {MPACK_EXPECT_RANGE_IMPL(float, float)}
double mpack_expect_double_range(mpack_reader_t* reader, double min_value, double max_value) {MPACK_EXPECT_RANGE_IMPL(double, double)}

uint32_t mpack_expect_map_range(mpack_reader_t* reader, uint32_t min_value, uint32_t max_value) {MPACK_EXPECT_RANGE_IMPL(map, uint32_t)}
uint32_t mpack_expect_array_range(mpack_reader_t* reader, uint32_t min_value, uint32_t max_value) {MPACK_EXPECT_RANGE_IMPL(array, uint32_t)}


// Matching Number Functions

void mpack_expect_uint_match(mpack_reader_t* reader, uint64_t value) {
    if (mpack_expect_u64(reader) != value)
        mpack_reader_flag_error(reader, mpack_error_type);
}

void mpack_expect_int_match(mpack_reader_t* reader, int64_t value) {
    if (mpack_expect_i64(reader) != value)
        mpack_reader_flag_error(reader, mpack_error_type);
}


// Other Basic Types

void mpack_expect_nil(mpack_reader_t* reader) {
    mpack_reader_track_element(reader);
    uint8_t type = mpack_read_native_u8(reader);
    if (reader->error != mpack_ok)
        return;
    if (type != 0xc0)
        mpack_reader_flag_error(reader, mpack_error_type);
}

bool mpack_expect_bool(mpack_reader_t* reader) {
    mpack_reader_track_element(reader);
    uint8_t type = mpack_read_native_u8(reader);
    if (reader->error != mpack_ok)
        return false;
    if ((type & ~1) != 0xc2)
        mpack_reader_flag_error(reader, mpack_error_type);
    return (bool)(type & 1);
}

void mpack_expect_true(mpack_reader_t* reader) {
    if (mpack_expect_bool(reader) != true)
        mpack_reader_flag_error(reader, mpack_error_type);
}

void mpack_expect_false(mpack_reader_t* reader) {
    if (mpack_expect_bool(reader) != false)
        mpack_reader_flag_error(reader, mpack_error_type);
}


// Compound Types

uint32_t mpack_expect_map(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_map)
        return var.v.n;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

void mpack_expect_map_match(mpack_reader_t* reader, uint32_t count) {
    if (mpack_expect_map(reader) != count)
        mpack_reader_flag_error(reader, mpack_error_type);
}

bool mpack_expect_map_or_nil(mpack_reader_t* reader, uint32_t* count) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_nil) {
        *count = 0;
        return false;
    }
    if (var.type == mpack_type_map) {
        *count = var.v.n;
        return true;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    *count = 0;
    return false;
}

bool mpack_expect_map_max_or_nil(mpack_reader_t* reader, uint32_t max_count, uint32_t* count) {
    bool has_map = mpack_expect_map_or_nil(reader, count);
    if (has_map && *count > max_count) {
        *count = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
        return false;
    }
    return has_map;
}

uint32_t mpack_expect_array(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_array)
        return var.v.n;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

void mpack_expect_array_match(mpack_reader_t* reader, uint32_t count) {
    if (mpack_expect_array(reader) != count)
        mpack_reader_flag_error(reader, mpack_error_type);
}

bool mpack_expect_array_or_nil(mpack_reader_t* reader, uint32_t* count) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_nil) {
        *count = 0;
        return false;
    }
    if (var.type == mpack_type_array) {
        *count = var.v.n;
        return true;
    }
    mpack_reader_flag_error(reader, mpack_error_type);
    *count = 0;
    return false;
}

bool mpack_expect_array_max_or_nil(mpack_reader_t* reader, uint32_t max_count, uint32_t* count) {
    bool has_array = mpack_expect_array_or_nil(reader, count);
    if (has_array && *count > max_count) {
        *count = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
        return false;
    }
    return has_array;
}

#ifdef MPACK_MALLOC
void* mpack_expect_array_alloc_impl(mpack_reader_t* reader, size_t element_size, uint32_t max_count, uint32_t* out_count, bool allow_nil) {
    *out_count = 0;

    uint32_t count;
    bool has_array = true;
    if (allow_nil)
        has_array = mpack_expect_array_max_or_nil(reader, max_count, &count);
    else
        count = mpack_expect_array_max(reader, max_count);
    if (mpack_reader_error(reader))
        return NULL;

    // size 0 is not an error; we return NULL for no elements.
    if (count == 0) {
        // we call mpack_done_array() automatically ONLY if we are using
        // the _or_nil variant. this is the only way to allow nil and empty
        // to work the same way.
        if (allow_nil && has_array)
            mpack_done_array(reader);
        return NULL;
    }

    void* p = MPACK_MALLOC(element_size * count);
    if (p == NULL) {
        mpack_reader_flag_error(reader, mpack_error_memory);
        return NULL;
    }

    *out_count = count;
    return p;
}
#endif


// Str, Bin and Ext Functions

uint32_t mpack_expect_str(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_str)
        return var.v.l;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

size_t mpack_expect_str_buf(mpack_reader_t* reader, char* buf, size_t bufsize) {
    size_t length = mpack_expect_str(reader);
    if (mpack_reader_error(reader))
        return 0;

    if (length > bufsize) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return 0;
    }

    mpack_read_bytes(reader, buf, length);
    if (mpack_reader_error(reader))
        return 0;

    mpack_done_str(reader);
    return length;
}

size_t mpack_expect_utf8(mpack_reader_t* reader, char* buf, size_t size) {
    size_t length = mpack_expect_str_buf(reader, buf, size);

    if (!mpack_utf8_check(buf, length)) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return 0;
    }

    return length;
}

uint32_t mpack_expect_bin(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_bin)
        return var.v.l;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

size_t mpack_expect_bin_buf(mpack_reader_t* reader, char* buf, size_t bufsize) {
    size_t binsize = mpack_expect_bin(reader);
    if (mpack_reader_error(reader))
        return 0;
    if (binsize > bufsize) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return 0;
    }
    mpack_read_bytes(reader, buf, binsize);
    if (mpack_reader_error(reader))
        return 0;
    mpack_done_bin(reader);
    return binsize;
}

static size_t mpack_expect_cstr_unchecked(mpack_reader_t* reader, char* buf, size_t bufsize) {

    // make sure buffer makes sense
    mpack_assert(bufsize >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    // expect a str
    size_t length = mpack_expect_str_buf(reader, buf, bufsize - 1);
    if (mpack_reader_error(reader)) {
        buf[0] = 0;
        return 0;
    }

    buf[length] = 0;
    return length;
}

void mpack_expect_cstr(mpack_reader_t* reader, char* buf, size_t bufsize) {
    size_t length = mpack_expect_cstr_unchecked(reader, buf, bufsize);

    // check for null bytes
    if (!mpack_str_check_no_null(buf, length)) {
        buf[0] = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
    }
}

void mpack_expect_utf8_cstr(mpack_reader_t* reader, char* buf, size_t bufsize) {
    size_t length = mpack_expect_cstr_unchecked(reader, buf, bufsize);

    // check encoding
    if (!mpack_utf8_check_no_null(buf, length)) {
        buf[0] = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
    }
}

#ifdef MPACK_MALLOC
char* mpack_expect_str_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size) {
    *size = 0;

    if (maxsize > UINT32_MAX)
        maxsize = UINT32_MAX;

    size_t length = mpack_expect_str_max(reader, (uint32_t)maxsize);
    char* str = mpack_read_bytes_alloc(reader, length);
    mpack_done_str(reader);

    if (str)
        *size = length;
    return str;
}

char* mpack_expect_utf8_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size) {
    char* str = mpack_expect_str_alloc(reader, maxsize, size);

    if (str && !mpack_utf8_check(str, *size)) {
        *size = 0;
        MPACK_FREE(str);
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }

    return str;
}

static char* mpack_expect_cstr_alloc_unchecked(mpack_reader_t* reader, size_t maxsize, size_t* out_length) {
    *out_length = 0;

    // make sure argument makes sense
    if (maxsize < 1) {
        mpack_break("maxsize is zero; you must have room for at least a null-terminator");
        mpack_reader_flag_error(reader, mpack_error_bug);
        return NULL;
    }

    if (maxsize > UINT32_MAX)
        maxsize = UINT32_MAX;

    size_t length = mpack_expect_str_max(reader, (uint32_t)maxsize - 1);
    char* str = mpack_read_bytes_alloc_size(reader, length, length + 1);
    mpack_done_str(reader);

    if (str) {
        str[length] = 0;
        *out_length = length;
    }
    return str;
}

char* mpack_expect_cstr_alloc(mpack_reader_t* reader, size_t maxsize) {
    size_t length;
    char* str = mpack_expect_cstr_alloc_unchecked(reader, maxsize, &length);

    if (str && !mpack_str_check_no_null(str, length)) {
        MPACK_FREE(str);
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }

    return str;
}

char* mpack_expect_utf8_cstr_alloc(mpack_reader_t* reader, size_t maxsize) {
    size_t length;
    char* str = mpack_expect_cstr_alloc_unchecked(reader, maxsize, &length);

    if (str && !mpack_utf8_check_no_null(str, length)) {
        MPACK_FREE(str);
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }

    return str;
}
#endif

void mpack_expect_str_match(mpack_reader_t* reader, const char* str, size_t len) {

    // expect a str the correct length
    if (len > UINT32_MAX)
        mpack_reader_flag_error(reader, mpack_error_type);
    mpack_expect_str_length(reader, (uint32_t)len);
    if (mpack_reader_error(reader))
        return;

    // check each byte
    for (size_t i = 0; i < len; ++i) {
        mpack_reader_track_bytes(reader, 1);
        if (mpack_read_native_u8(reader) != *str++) {
            mpack_reader_flag_error(reader, mpack_error_type);
            return;
        }
    }

    mpack_done_str(reader);
}

void mpack_expect_tag(mpack_reader_t* reader, mpack_tag_t expected) {
    mpack_tag_t actual = mpack_read_tag(reader);
    if (!mpack_tag_equal(actual, expected))
        mpack_reader_flag_error(reader, mpack_error_type);
}

#ifdef MPACK_MALLOC
char* mpack_expect_bin_alloc(mpack_reader_t* reader, size_t maxsize, size_t* size) {
    *size = 0;

    if (maxsize > UINT32_MAX)
        maxsize = UINT32_MAX;

    size_t length = mpack_expect_bin_max(reader, (uint32_t)maxsize);
    char* data = mpack_read_bytes_alloc(reader, length);
    mpack_done_bin(reader);

    if (data)
        *size = length;
    return data;
}
#endif

#endif

