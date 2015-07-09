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

// TODO: these can be made more efficient. they don't need to read a full
// tag; they should just directly read bytes and parse what they are actually
// looking for.

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

int8_t mpack_expect_i8_range(mpack_reader_t* reader, int8_t min_value, int8_t max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %i must be less than or equal to max_value %i",
            min_value, max_value);

    // read the value
    int8_t val = mpack_expect_i8(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

// TODO: missing i16_range, i32_range, i64_range?

uint8_t mpack_expect_u8_range(mpack_reader_t* reader, uint8_t min_value, uint8_t max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %u must be less than or equal to max_value %u",
            min_value, max_value);

    // read the value
    uint8_t val = mpack_expect_u8(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

uint16_t mpack_expect_u16_range(mpack_reader_t* reader, uint16_t min_value, uint16_t max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %u must be less than or equal to max_value %u",
            min_value, max_value);

    // read the value
    uint16_t val = mpack_expect_u16(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

uint32_t mpack_expect_u32_range(mpack_reader_t* reader, uint32_t min_value, uint32_t max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %u must be less than or equal to max_value %u",
            min_value, max_value);

    // read the value
    uint32_t val = mpack_expect_u32(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

uint64_t mpack_expect_u64_range(mpack_reader_t* reader, uint64_t min_value, uint64_t max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value,
            "min_value %"PRIu64" must be less than or equal to max_value %"PRIu64, min_value, max_value);


    // read the value
    uint64_t val = mpack_expect_u64(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

float mpack_expect_float_range(mpack_reader_t* reader, float min_value, float max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %f must be less than or equal to max_value %f",
            min_value, max_value);

    // read the value
    float val = mpack_expect_float(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}

double mpack_expect_double_range(mpack_reader_t* reader, double min_value, double max_value) {

    // make sure the range is sensible
    mpack_assert(min_value <= max_value, "min_value %f must be less than or equal to max_value %f",
            min_value, max_value);

    // read the value
    double val = mpack_expect_double(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_value;

    // make sure it fits
    if (val < min_value || val > max_value) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_value;
    }

    return val;
}


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

uint32_t mpack_expect_array_range(mpack_reader_t* reader, uint32_t min_count, uint32_t max_count) {

    // make sure the range is sensible
    mpack_assert(min_count <= max_count, "min_count %u must be less than or equal to max_count %u",
            min_count, max_count);

    // read the count
    uint32_t count = mpack_expect_array(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return min_count;

    // make sure it fits
    if (count < min_count || count > max_count) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return min_count;
    }

    return count;
}

#ifdef MPACK_MALLOC
void* mpack_expect_array_alloc_impl(mpack_reader_t* reader, size_t element_size, uint32_t max_count, size_t* out_count) {
    size_t count = *out_count = mpack_expect_array(reader);
    if (mpack_reader_error(reader))
        return NULL;
    if (count > max_count) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }
    void* p = MPACK_MALLOC(element_size * count);
    if (p == NULL)
        mpack_reader_flag_error(reader, mpack_error_memory);
    return p;
}
#endif


// String Functions

uint32_t mpack_expect_str(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (var.type == mpack_type_str)
        return var.v.l;
    mpack_reader_flag_error(reader, mpack_error_type);
    return 0;
}

size_t mpack_expect_str_buf(mpack_reader_t* reader, char* buf, size_t bufsize) {
    size_t strsize = mpack_expect_str(reader);
    if (mpack_reader_error(reader))
        return 0;
    if (strsize > bufsize) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return 0;
    }
    mpack_read_bytes(reader, buf, strsize);
    if (mpack_reader_error(reader))
        return 0;
    mpack_done_str(reader);
    return strsize;
}


// Binary Blob Functions

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

void mpack_expect_cstr(mpack_reader_t* reader, char* buf, size_t bufsize) {

    // make sure buffer makes sense
    mpack_assert(bufsize >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    // expect a str
    size_t rawsize = mpack_expect_str_buf(reader, buf, bufsize - 1);
    if (mpack_reader_error(reader)) {
        buf[0] = 0;
        return;
    }
    buf[rawsize] = 0;

    // check it for null bytes
    for (size_t i = 0; i < rawsize; ++i) {
        if (buf[i] == 0) {
            buf[0] = 0;
            mpack_reader_flag_error(reader, mpack_error_type);
            return;
        }
    }
}

void mpack_expect_utf8_cstr(mpack_reader_t* reader, char* buf, size_t bufsize) {

    // make sure buffer makes sense
    mpack_assert(bufsize >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    // expect a raw
    size_t rawsize = mpack_expect_str_buf(reader, buf, bufsize - 1);
    if (mpack_reader_error(reader)) {
        buf[0] = 0;
        return;
    }
    buf[rawsize] = 0;

    // check encoding
    uint32_t state = 0;
    uint32_t codepoint = 0;
    for (size_t i = 0; i < rawsize; ++i) {
        if (mpack_utf8_decode(&state, &codepoint, buf[i]) == MPACK_UTF8_REJECT) {
            buf[0] = 0;
            mpack_reader_flag_error(reader, mpack_error_type);
            return;
        }
    }

}

#ifdef MPACK_MALLOC
char* mpack_expect_cstr_alloc(mpack_reader_t* reader, size_t maxsize) {

    // make sure argument makes sense
    if (maxsize < 1) {
        mpack_break("maxsize is zero; you must have room for at least a null-terminator");
        mpack_reader_flag_error(reader, mpack_error_bug);
        return NULL;
    }

    // read size
    size_t length = mpack_expect_str(reader); // TODO: use expect str max? create expect str max...
    if (mpack_reader_error(reader))
        return NULL;
    if (length > (maxsize - 1)) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }

    // allocate
    char* str = (char*)MPACK_MALLOC(length + 1);
    if (str == NULL) {
        mpack_reader_flag_error(reader, mpack_error_memory);
        return NULL;
    }

    // read with jump disabled so we don't leak our buffer
    mpack_reader_track_bytes(reader, length);
    mpack_read_native_nojump(reader, str, length);

    if (mpack_reader_error(reader)) {
        MPACK_FREE(str);
        return NULL;
    }
    str[length] = 0;
    mpack_done_str(reader);
    return str;
}
#endif

void mpack_expect_cstr_match(mpack_reader_t* reader, const char* str) {
    if (reader->error != mpack_ok)
        return;

    // expect a str the correct length
    size_t len = mpack_strlen(str);
    if (len > UINT32_MAX)
        mpack_reader_flag_error(reader, mpack_error_invalid);
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


#endif

