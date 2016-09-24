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

#define MPACK_INTERNAL 1

#include "mpack-reader.h"

#if MPACK_READER

static void mpack_reader_skip_using_fill(mpack_reader_t* reader, size_t count);

void mpack_reader_init(mpack_reader_t* reader, char* buffer, size_t size, size_t count) {
    mpack_assert(buffer != NULL, "buffer is NULL");

    mpack_memset(reader, 0, sizeof(*reader));
    reader->buffer = buffer;
    reader->size = size;
    reader->data = buffer;
    reader->left = count;

    #if MPACK_READ_TRACKING
    mpack_reader_flag_if_error(reader, mpack_track_init(&reader->track));
    #endif

    mpack_log("===========================\n");
    mpack_log("initializing reader with buffer size %i\n", (int)size);
}

void mpack_reader_init_error(mpack_reader_t* reader, mpack_error_t error) {
    mpack_memset(reader, 0, sizeof(*reader));
    reader->error = error;

    mpack_log("===========================\n");
    mpack_log("initializing reader error state %i\n", (int)error);
}

void mpack_reader_init_data(mpack_reader_t* reader, const char* data, size_t count) {
    mpack_assert(data != NULL, "data is NULL");

    mpack_memset(reader, 0, sizeof(*reader));
    reader->data = data;
    reader->left = count;

    #if MPACK_READ_TRACKING
    mpack_reader_flag_if_error(reader, mpack_track_init(&reader->track));
    #endif

    mpack_log("===========================\n");
    mpack_log("initializing reader with data size %i\n", (int)count);
}

void mpack_reader_set_fill(mpack_reader_t* reader, mpack_reader_fill_t fill) {
    MPACK_STATIC_ASSERT(MPACK_READER_MINIMUM_BUFFER_SIZE >= MPACK_MAXIMUM_TAG_SIZE,
            "minimum buffer size must fit any tag!");

    if (reader->size == 0) {
        mpack_break("cannot use fill function without a writeable buffer!");
        mpack_reader_flag_error(reader, mpack_error_bug);
        return;
    }

    if (reader->size < MPACK_READER_MINIMUM_BUFFER_SIZE) {
        mpack_break("buffer size is %i, but minimum buffer size for fill is %i",
                (int)reader->size, MPACK_READER_MINIMUM_BUFFER_SIZE);
        mpack_reader_flag_error(reader, mpack_error_bug);
        return;
    }

    reader->fill = fill;
}

void mpack_reader_set_skip(mpack_reader_t* reader, mpack_reader_skip_t skip) {
    mpack_assert(reader->size != 0, "cannot use skip function without a writeable buffer!");
    reader->skip = skip;
}

#if MPACK_STDIO
static size_t mpack_file_reader_fill(mpack_reader_t* reader, char* buffer, size_t count) {
    if (feof((FILE *)reader->context)) {
       mpack_reader_flag_error(reader, mpack_error_eof);
       return 0;
    }
    return fread((void*)buffer, 1, count, (FILE*)reader->context);
}

static void mpack_file_reader_skip(mpack_reader_t* reader, size_t count) {
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    FILE* file = (FILE*)reader->context;

    // We call ftell() to test whether the stream is seekable
    // without causing a file error.
    if (ftell(file) >= 0) {
        mpack_log("seeking forward %i bytes\n", (int)count);
        if (fseek(file, (long int)count, SEEK_CUR) == 0)
            return;
        mpack_log("fseek() didn't return zero!\n");
        if (ferror(file)) {
            mpack_reader_flag_error(reader, mpack_error_io);
            return;
        }
    }

    // If the stream is not seekable, fall back to the fill function.
    mpack_reader_skip_using_fill(reader, count);
}

static void mpack_file_reader_teardown(mpack_reader_t* reader) {
    FILE* file = (FILE*)reader->context;

    if (file) {
        int ret = fclose(file);
        reader->context = NULL;
        if (ret != 0)
            mpack_reader_flag_error(reader, mpack_error_io);
    }

    MPACK_FREE(reader->buffer);
    reader->buffer = NULL;
    reader->size = 0;
    reader->fill = NULL;
    reader->skip = NULL;
    reader->teardown = NULL;
}

void mpack_reader_init_file(mpack_reader_t* reader, const char* filename) {
    mpack_assert(filename != NULL, "filename is NULL");

    size_t capacity = MPACK_BUFFER_SIZE;
    char* buffer = (char*)MPACK_MALLOC(capacity);
    if (buffer == NULL) {
        mpack_reader_init_error(reader, mpack_error_memory);
        return;
    }

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        MPACK_FREE(buffer);
        mpack_reader_init_error(reader, mpack_error_io);
        return;
    }

    mpack_reader_init(reader, buffer, capacity, 0);
    mpack_reader_set_context(reader, file);
    mpack_reader_set_fill(reader, mpack_file_reader_fill);
    mpack_reader_set_skip(reader, mpack_file_reader_skip);
    mpack_reader_set_teardown(reader, mpack_file_reader_teardown);
}
#endif

mpack_error_t mpack_reader_destroy(mpack_reader_t* reader) {

    // clean up tracking, asserting if we're not already in an error state
    #if MPACK_READ_TRACKING
    mpack_reader_flag_if_error(reader, mpack_track_destroy(&reader->track, mpack_reader_error(reader) != mpack_ok));
    #endif

    if (reader->teardown)
        reader->teardown(reader);
    reader->teardown = NULL;

    return reader->error;
}

size_t mpack_reader_remaining(mpack_reader_t* reader, const char** data) {
    if (mpack_reader_error(reader) != mpack_ok)
        return 0;

    #if MPACK_READ_TRACKING
    if (mpack_reader_flag_if_error(reader, mpack_track_check_empty(&reader->track)) != mpack_ok)
        return 0;
    #endif

    if (data)
        *data = reader->data;
    return reader->left;
}

void mpack_reader_flag_error(mpack_reader_t* reader, mpack_error_t error) {
    mpack_log("reader %p setting error %i: %s\n", reader, (int)error, mpack_error_to_string(error));

    if (reader->error == mpack_ok) {
        reader->error = error;
        reader->left = 0;
        if (reader->error_fn)
            reader->error_fn(reader, error);
    }
}

// A helper to call the reader fill function. This makes sure it's
// implemented and guards against overflow in case it returns -1.
static size_t mpack_fill(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(reader->fill != NULL, "mpack_fill() called with no fill function?");

    size_t ret = reader->fill(reader, p, count);
    if (ret == ((size_t)(-1)))
        return 0;

    return ret;
}

// Loops on the fill function, reading between the minimum and
// maximum number of bytes and flagging an error if it fails.
static size_t mpack_fill_range(mpack_reader_t* reader, char* p, size_t min_bytes, size_t max_bytes) {
    mpack_assert(min_bytes > 0, "cannot fill zero bytes!");
    mpack_assert(max_bytes >= min_bytes, "min_bytes %i cannot be larger than max_bytes %i!",
            (int)min_bytes, (int)max_bytes);

    size_t count = 0;
    while (count < min_bytes) {
        size_t read = mpack_fill(reader, p + count, max_bytes - count);
        if (mpack_reader_error(reader) != mpack_ok)
            return 0;
        if (read == 0) {
            mpack_reader_flag_error(reader, mpack_error_io);
            return 0;
        }
        count += read;
    }
    return count;
}

bool mpack_reader_ensure_straddle(mpack_reader_t* reader, size_t count) {
    mpack_assert(count != 0, "cannot ensure zero bytes!");
    mpack_assert(reader->error == mpack_ok, "reader cannot be in an error state!");

    mpack_assert(count > reader->left,
            "straddling ensure requested for %i bytes, but there are %i bytes "
            "left in buffer. call mpack_reader_ensure() instead",
            (int)count, (int)reader->left);

    // we'll need a fill function to get more data. if there's no
    // fill function, the buffer should contain an entire MessagePack
    // object, so we raise mpack_error_invalid instead of mpack_error_io
    // on truncated data.
    if (reader->fill == NULL) {
        mpack_reader_flag_error(reader, mpack_error_invalid);
        return false;
    }

    // we need enough space in the buffer. if the buffer is not
    // big enough, we return mpack_error_too_big (since this is
    // for an in-place read larger than the buffer size.)
    if (count > reader->size) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return false;
    }

    // move the existing data to the start of the buffer
    mpack_memmove(reader->buffer, reader->data, reader->left);
    reader->data = reader->buffer;

    // read at least the necessary number of bytes, accepting up to the
    // buffer size
    size_t read = mpack_fill_range(reader, reader->buffer + reader->left,
            count - reader->left, reader->size - reader->left);
    if (mpack_reader_error(reader) != mpack_ok)
        return false;
    reader->left += read;
    return true;
}

// Reads count bytes into p. Used when there are not enough bytes
// left in the buffer to satisfy a read.
void mpack_read_native_big(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (mpack_reader_error(reader) != mpack_ok) {
        mpack_memset(p, 0, count);
        return;
    }

    mpack_log("big read for %i bytes into %p, %i left in buffer, buffer size %i\n",
            (int)count, p, (int)reader->left, (int)reader->size);

    if (count <= reader->left) {
        mpack_assert(0,
                "big read requested for %i bytes, but there are %i bytes "
                "left in buffer. call mpack_read_native() instead",
                (int)count, (int)reader->left);
        mpack_reader_flag_error(reader, mpack_error_bug);
        mpack_memset(p, 0, count);
        return;
    }

    // we'll need a fill function to get more data. if there's no
    // fill function, the buffer should contain an entire MessagePack
    // object, so we raise mpack_error_invalid instead of mpack_error_io
    // on truncated data.
    if (reader->fill == NULL) {
        mpack_reader_flag_error(reader, mpack_error_invalid);
        mpack_memset(p, 0, count);
        return;
    }

    if (reader->size == 0) {
        // somewhat debatable what error should be returned here. when
        // initializing a reader with an in-memory buffer it's not
        // necessarily a bug if the data is blank; it might just have
        // been truncated to zero. for this reason we return the same
        // error as if the data was truncated.
        mpack_reader_flag_error(reader, mpack_error_io);
        mpack_memset(p, 0, count);
        return;
    }

    // flush what's left of the buffer
    if (reader->left > 0) {
        mpack_log("flushing %i bytes remaining in buffer\n", (int)reader->left);
        mpack_memcpy(p, reader->data, reader->left);
        count -= reader->left;
        p += reader->left;
        reader->data += reader->left;
        reader->left = 0;
    }

    // if the remaining data needed is some small fraction of the
    // buffer size, we'll try to fill the buffer as much as possible
    // and copy the needed data out.
    if (count <= reader->size / MPACK_READER_SMALL_FRACTION_DENOMINATOR) {
        size_t read = mpack_fill_range(reader, reader->buffer, count, reader->size);
        if (mpack_reader_error(reader) != mpack_ok)
            return;
        mpack_memcpy(p, reader->buffer, count);
        reader->data = reader->buffer + count;
        reader->left = read - count;

    // otherwise we read the remaining data directly into the target.
    } else {
        mpack_log("reading %i additional bytes\n", (int)reader->left);
        mpack_fill_range(reader, p, count, count);
    }
}

void mpack_skip_bytes(mpack_reader_t* reader, size_t count) {
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    mpack_log("skip requested for %i bytes\n", (int)count);
    mpack_reader_track_bytes(reader, count);

    // check if we have enough in the buffer already
    if (reader->left >= count) {
        mpack_log("skipping %i bytes still in buffer\n", (int)count);
        reader->left -= count;
        reader->data += count;
        return;
    }

    // we'll need at least a fill function to skip more data. if there's
    // no fill function, the buffer should contain an entire MessagePack
    // object, so we raise mpack_error_invalid instead of mpack_error_io
    // on truncated data. (see mpack_read_native_big())
    if (reader->fill == NULL) {
        mpack_log("reader has no fill function!\n");
        mpack_reader_flag_error(reader, mpack_error_invalid);
        return;
    }

    // discard whatever's left in the buffer
    mpack_log("discarding %i bytes still in buffer\n", (int)reader->left);
    count -= reader->left;
    reader->left = 0;

    // use the skip function if we've got one, and if we're trying
    // to skip a lot of data. if we only need to skip some tiny
    // fraction of the buffer size, it's probably better to just
    // fill the buffer and skip from it instead of trying to seek.
    if (reader->skip && count > reader->size / 16) {
        mpack_log("calling skip function for %i bytes\n", (int)count);
        reader->skip(reader, count);
        return;
    }

    mpack_reader_skip_using_fill(reader, count);
}

static void mpack_reader_skip_using_fill(mpack_reader_t* reader, size_t count) {
    mpack_assert(reader->fill != NULL, "missing fill function!");
    mpack_assert(reader->left == 0, "there are bytes left in the buffer!");
    mpack_assert(reader->error == mpack_ok, "should not have called this in an error state (%i)", reader->error);
    mpack_log("skip using fill for %i bytes\n", (int)count);

    // fill and discard multiples of the buffer size
    while (count > reader->size) {
        mpack_log("filling and discarding buffer of %i bytes\n", (int)reader->size);
        if (mpack_fill_range(reader, reader->buffer, reader->size, reader->size) < reader->size) {
            mpack_reader_flag_error(reader, mpack_error_io);
            return;
        }
        count -= reader->size;
    }

    // fill the buffer as much as possible
    reader->data = reader->buffer;
    reader->left = mpack_fill_range(reader, reader->buffer, count, reader->size);
    if (reader->left < count) {
        mpack_reader_flag_error(reader, mpack_error_io);
        return;
    }
    mpack_log("filled %i bytes into buffer; discarding %i bytes\n", (int)reader->left, (int)count);
    reader->data += count;
    reader->left -= count;
}

void mpack_read_bytes(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(p != NULL, "destination for read of %i bytes is NULL", (int)count);
    mpack_reader_track_bytes(reader, count);
    mpack_read_native(reader, p, count);
}

void mpack_read_utf8(mpack_reader_t* reader, char* p, size_t byte_count) {
    mpack_assert(p != NULL, "destination for read of %i bytes is NULL", (int)byte_count);
    mpack_reader_track_str_bytes_all(reader, byte_count);
    mpack_read_native(reader, p, byte_count);

    if (mpack_reader_error(reader) == mpack_ok && !mpack_utf8_check(p, byte_count))
        mpack_reader_flag_error(reader, mpack_error_type);
}

static void mpack_read_cstr_unchecked(mpack_reader_t* reader, char* buf, size_t buffer_size, size_t byte_count) {
    mpack_assert(buf != NULL, "destination for read of %i bytes is NULL", (int)byte_count);
    mpack_assert(buffer_size >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    if (mpack_reader_error(reader)) {
        buf[0] = 0;
        return;
    }

    if (byte_count > buffer_size - 1) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        buf[0] = 0;
        return;
    }

    mpack_reader_track_str_bytes_all(reader, byte_count);
    mpack_read_native(reader, buf, byte_count);
    buf[byte_count] = 0;
}

void mpack_read_cstr(mpack_reader_t* reader, char* buf, size_t buffer_size, size_t byte_count) {
    mpack_read_cstr_unchecked(reader, buf, buffer_size, byte_count);

    // check for null bytes
    if (mpack_reader_error(reader) == mpack_ok && !mpack_str_check_no_null(buf, byte_count)) {
        buf[0] = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
    }
}

void mpack_read_utf8_cstr(mpack_reader_t* reader, char* buf, size_t buffer_size, size_t byte_count) {
    mpack_read_cstr_unchecked(reader, buf, buffer_size, byte_count);

    // check encoding
    if (mpack_reader_error(reader) == mpack_ok && !mpack_utf8_check_no_null(buf, byte_count)) {
        buf[0] = 0;
        mpack_reader_flag_error(reader, mpack_error_type);
    }
}

#ifdef MPACK_MALLOC
// Reads native bytes with error callback disabled. This allows MPack reader functions
// to hold an allocated buffer and read native data into it without leaking it in
// case of a non-local jump (longjmp, throw) out of an error handler.
static void mpack_read_native_noerrorfn(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(reader->error == mpack_ok, "cannot call if an error is already flagged!");
    mpack_reader_error_t error_fn = reader->error_fn;
    reader->error_fn = NULL;
    mpack_read_native(reader, p, count);
    reader->error_fn = error_fn;
}

char* mpack_read_bytes_alloc_impl(mpack_reader_t* reader, size_t count, bool null_terminated) {

    // track the bytes first in case it jumps
    mpack_reader_track_bytes(reader, count);
    if (mpack_reader_error(reader) != mpack_ok)
        return NULL;

    // cannot allocate zero bytes. this is not an error.
    if (count == 0 && null_terminated == false)
        return NULL;

    // allocate data
    char* data = (char*)MPACK_MALLOC(count + (null_terminated ? 1 : 0)); // TODO: can this overflow?
    if (data == NULL) {
        mpack_reader_flag_error(reader, mpack_error_memory);
        return NULL;
    }

    // read with error callback disabled so we don't leak our buffer
    mpack_read_native_noerrorfn(reader, data, count);

    // report flagged errors
    if (mpack_reader_error(reader) != mpack_ok) {
        MPACK_FREE(data);
        if (reader->error_fn)
            reader->error_fn(reader, mpack_reader_error(reader));
        return NULL;
    }

    if (null_terminated)
        data[count] = '\0';
    return data;
}
#endif

// read inplace without tracking (since there are different
// tracking modes for different inplace readers)
static const char* mpack_read_bytes_inplace_notrack(mpack_reader_t* reader, size_t count) {
    if (mpack_reader_error(reader) != mpack_ok)
        return NULL;

    // if we have enough bytes already in the buffer, we can return it directly.
    if (reader->left >= count) {
        reader->data += count;
        reader->left -= count;
        return reader->data - count;
    }

    if (!mpack_reader_ensure(reader, count))
        return NULL;

    reader->data += count;
    reader->left -= count;
    return reader->data - count;
}

const char* mpack_read_bytes_inplace(mpack_reader_t* reader, size_t count) {
    mpack_reader_track_bytes(reader, count);
    return mpack_read_bytes_inplace_notrack(reader, count);
}

const char* mpack_read_utf8_inplace(mpack_reader_t* reader, size_t count) {
    mpack_reader_track_str_bytes_all(reader, count);
    const char* str = mpack_read_bytes_inplace_notrack(reader, count);

    if (mpack_reader_error(reader) == mpack_ok && !mpack_utf8_check(str, count)) {
        mpack_reader_flag_error(reader, mpack_error_type);
        return NULL;
    }

    return str;
}

static size_t mpack_parse_tag(mpack_reader_t* reader, mpack_tag_t* tag) {
    mpack_assert(reader->error == mpack_ok, "reader cannot be in an error state!");

    if (!mpack_reader_ensure(reader, 1))
        return 0;
    uint8_t type = mpack_load_u8(reader->data);

    // unfortunately, by far the fastest way to parse a tag is to switch
    // on the first byte, and to explicitly list every possible byte. so for
    // infix types, the list of cases is quite large.
    //
    // in size-optimized builds, we switch on the top four bits first to
    // handle most infix types with a smaller jump table to save space.

    #if MPACK_OPTIMIZE_FOR_SIZE
    switch (type >> 4) {

        // positive fixnum
        case 0x0: case 0x1: case 0x2: case 0x3:
        case 0x4: case 0x5: case 0x6: case 0x7:
            tag->type = mpack_type_uint;
            tag->v.u = type;
            return 1;

        // negative fixnum
        case 0xe: case 0xf:
            tag->type = mpack_type_int;
            tag->v.i = (int32_t)(int8_t)type;
            return 1;

        // fixmap
        case 0x8:
            tag->type = mpack_type_map;
            tag->v.n = (uint32_t)(type & ~0xf0);
            return 1;

        // fixarray
        case 0x9:
            tag->type = mpack_type_array;
            tag->v.n = (uint32_t)(type & ~0xf0);
            return 1;

        // fixstr
        case 0xa: case 0xb:
            tag->type = mpack_type_str;
            tag->v.l = (uint32_t)(type & ~0xe0);
            return 1;

        // not one of the common infix types
        default:
            break;

    }
    #endif

    // handle individual type tags
    switch (type) {

        #if !MPACK_OPTIMIZE_FOR_SIZE
        // positive fixnum
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
            tag->type = mpack_type_uint;
            tag->v.u = type;
            return 1;

        // negative fixnum
        case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
        case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
            tag->type = mpack_type_int;
            tag->v.i = (int8_t)type;
            return 1;

        // fixmap
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
            tag->type = mpack_type_map;
            tag->v.n = (uint32_t)(type & ~0xf0);
            return 1;

        // fixarray
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            tag->type = mpack_type_array;
            tag->v.n = (uint32_t)(type & ~0xf0);
            return 1;

        // fixstr
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            tag->type = mpack_type_str;
            tag->v.l = (uint32_t)(type & ~0xe0);
            return 1;
        #endif

        // nil
        case 0xc0:
            tag->type = mpack_type_nil;
            return 1;

        // bool
        case 0xc2: case 0xc3:
            tag->type = mpack_type_bool;
            tag->v.b = type & 1;
            return 1;

        // bin8
        case 0xc4:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_BIN8))
                return 0;
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_u8(reader->data + 1);
            return MPACK_TAG_SIZE_BIN8;

        // bin16
        case 0xc5:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_BIN16))
                return 0;
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_u16(reader->data + 1);
            return MPACK_TAG_SIZE_BIN16;

        // bin32
        case 0xc6:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_BIN32))
                return 0;
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_u32(reader->data + 1);
            return MPACK_TAG_SIZE_BIN32;

        // ext8
        case 0xc7:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_EXT8))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_u8(reader->data + 1);
            tag->exttype = mpack_load_i8(reader->data + 2);
            return MPACK_TAG_SIZE_EXT8;

        // ext16
        case 0xc8:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_EXT16))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_u16(reader->data + 1);
            tag->exttype = mpack_load_i8(reader->data + 3);
            return MPACK_TAG_SIZE_EXT16;

        // ext32
        case 0xc9:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_EXT32))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_u32(reader->data + 1);
            tag->exttype = mpack_load_i8(reader->data + 5);
            return MPACK_TAG_SIZE_EXT32;

        // float
        case 0xca:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FLOAT))
                return 0;
            tag->type = mpack_type_float;
            tag->v.f = mpack_load_float(reader->data + 1);
            return MPACK_TAG_SIZE_FLOAT;

        // double
        case 0xcb:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_DOUBLE))
                return 0;
            tag->type = mpack_type_double;
            tag->v.d = mpack_load_double(reader->data + 1);
            return MPACK_TAG_SIZE_DOUBLE;

        // uint8
        case 0xcc:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_U8))
                return 0;
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_u8(reader->data + 1);
            return MPACK_TAG_SIZE_U8;

        // uint16
        case 0xcd:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_U16))
                return 0;
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_u16(reader->data + 1);
            return MPACK_TAG_SIZE_U16;

        // uint32
        case 0xce:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_U32))
                return 0;
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_u32(reader->data + 1);
            return MPACK_TAG_SIZE_U32;

        // uint64
        case 0xcf:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_U64))
                return 0;
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_u64(reader->data + 1);
            return MPACK_TAG_SIZE_U64;

        // int8
        case 0xd0:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_I8))
                return 0;
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_i8(reader->data + 1);
            return MPACK_TAG_SIZE_I8;

        // int16
        case 0xd1:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_I16))
                return 0;
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_i16(reader->data + 1);
            return MPACK_TAG_SIZE_I16;

        // int32
        case 0xd2:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_I32))
                return 0;
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_i32(reader->data + 1);
            return MPACK_TAG_SIZE_I32;

        // int64
        case 0xd3:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_I64))
                return 0;
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_i64(reader->data + 1);
            return MPACK_TAG_SIZE_I64;

        // fixext1
        case 0xd4:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FIXEXT1))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = 1;
            tag->exttype = mpack_load_i8(reader->data + 1);
            return MPACK_TAG_SIZE_FIXEXT1;

        // fixext2
        case 0xd5:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FIXEXT2))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = 2;
            tag->exttype = mpack_load_i8(reader->data + 1);
            return MPACK_TAG_SIZE_FIXEXT2;

        // fixext4
        case 0xd6:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FIXEXT4))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = 4;
            tag->exttype = mpack_load_i8(reader->data + 1);
            return 2;

        // fixext8
        case 0xd7:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FIXEXT8))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = 8;
            tag->exttype = mpack_load_i8(reader->data + 1);
            return MPACK_TAG_SIZE_FIXEXT8;

        // fixext16
        case 0xd8:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_FIXEXT16))
                return 0;
            tag->type = mpack_type_ext;
            tag->v.l = 16;
            tag->exttype = mpack_load_i8(reader->data + 1);
            return MPACK_TAG_SIZE_FIXEXT16;

        // str8
        case 0xd9:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_STR8))
                return 0;
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_u8(reader->data + 1);
            return MPACK_TAG_SIZE_STR8;

        // str16
        case 0xda:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_STR16))
                return 0;
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_u16(reader->data + 1);
            return MPACK_TAG_SIZE_STR16;

        // str32
        case 0xdb:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_STR32))
                return 0;
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_u32(reader->data + 1);
            return MPACK_TAG_SIZE_STR32;

        // array16
        case 0xdc:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_ARRAY16))
                return 0;
            tag->type = mpack_type_array;
            tag->v.n = mpack_load_u16(reader->data + 1);
            return MPACK_TAG_SIZE_ARRAY16;

        // array32
        case 0xdd:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_ARRAY32))
                return 0;
            tag->type = mpack_type_array;
            tag->v.n = mpack_load_u32(reader->data + 1);
            return MPACK_TAG_SIZE_ARRAY32;

        // map16
        case 0xde:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_MAP16))
                return 0;
            tag->type = mpack_type_map;
            tag->v.n = mpack_load_u16(reader->data + 1);
            return MPACK_TAG_SIZE_MAP16;

        // map32
        case 0xdf:
            if (!mpack_reader_ensure(reader, MPACK_TAG_SIZE_MAP32))
                return 0;
            tag->type = mpack_type_map;
            tag->v.n = mpack_load_u32(reader->data + 1);
            return MPACK_TAG_SIZE_MAP32;

        // reserved
        case 0xc1:
            break;

        #if MPACK_OPTIMIZE_FOR_SIZE
        // any other bytes should have been handled by the infix switch
        default:
            mpack_assert(0, "unreachable");
            break;
        #endif
    }

    // unrecognized type
    mpack_reader_flag_error(reader, mpack_error_invalid);
    return 0;
}

mpack_tag_t mpack_read_tag(mpack_reader_t* reader) {
    mpack_log("reading tag\n");

    // make sure we can read a tag
    if (mpack_reader_error(reader) != mpack_ok)
        return mpack_tag_nil();
    if (mpack_reader_track_element(reader) != mpack_ok)
        return mpack_tag_nil();

    mpack_tag_t tag;
    mpack_memset(&tag, 0, sizeof(tag));
    size_t count = mpack_parse_tag(reader, &tag);
    if (count == 0)
        return mpack_tag_nil();

    #if MPACK_READ_TRACKING
    mpack_error_t track_error = mpack_ok;

    switch (tag.type) {
        case mpack_type_map:
        case mpack_type_array:
            track_error = mpack_track_push(&reader->track, tag.type, tag.v.l);
            break;
        case mpack_type_str:
        case mpack_type_bin:
        case mpack_type_ext:
            track_error = mpack_track_push(&reader->track, tag.type, tag.v.n);
            break;
        default:
            break;
    }

    if (track_error != mpack_ok) {
        mpack_reader_flag_error(reader, track_error);
        return mpack_tag_nil();
    }
    #endif

    reader->data += count;
    reader->left -= count;
    return tag;
}

mpack_tag_t mpack_peek_tag(mpack_reader_t* reader) {
    mpack_log("peeking tag\n");

    // make sure we can peek a tag
    if (mpack_reader_error(reader) != mpack_ok)
        return mpack_tag_nil();
    if (mpack_reader_track_peek_element(reader) != mpack_ok)
        return mpack_tag_nil();

    mpack_tag_t tag;
    mpack_memset(&tag, 0, sizeof(tag));
    if (mpack_parse_tag(reader, &tag) == 0)
        return mpack_tag_nil();
    return tag;
}

void mpack_discard(mpack_reader_t* reader) {
    mpack_tag_t var = mpack_read_tag(reader);
    if (mpack_reader_error(reader))
        return;
    switch (var.type) {
        case mpack_type_str:
            mpack_skip_bytes(reader, var.v.l);
            mpack_done_str(reader);
            break;
        case mpack_type_bin:
            mpack_skip_bytes(reader, var.v.l);
            mpack_done_bin(reader);
            break;
        case mpack_type_ext:
            mpack_skip_bytes(reader, var.v.l);
            mpack_done_ext(reader);
            break;
        case mpack_type_array: {
            for (; var.v.n > 0; --var.v.n) {
                mpack_discard(reader);
                if (mpack_reader_error(reader))
                    break;
            }
            mpack_done_array(reader);
            break;
        }
        case mpack_type_map: {
            for (; var.v.n > 0; --var.v.n) {
                mpack_discard(reader);
                mpack_discard(reader);
                if (mpack_reader_error(reader))
                    break;
            }
            mpack_done_map(reader);
            break;
        }
        default:
            break;
    }
}

#if MPACK_READ_TRACKING
void mpack_done_type(mpack_reader_t* reader, mpack_type_t type) {
    if (mpack_reader_error(reader) == mpack_ok)
        mpack_reader_flag_if_error(reader, mpack_track_pop(&reader->track, type));
}
#endif

#if MPACK_STDIO
static void mpack_print_element(mpack_reader_t* reader, size_t depth, FILE* file) {
    mpack_tag_t val = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    switch (val.type) {

        case mpack_type_nil:
            fprintf(file, "null");
            break;
        case mpack_type_bool:
            fprintf(file, val.v.b ? "true" : "false");
            break;

        case mpack_type_float:
            fprintf(file, "%f", val.v.f);
            break;
        case mpack_type_double:
            fprintf(file, "%f", val.v.d);
            break;

        case mpack_type_int:
            fprintf(file, "%" PRIi64, val.v.i);
            break;
        case mpack_type_uint:
            fprintf(file, "%" PRIu64, val.v.u);
            break;

        case mpack_type_bin:
            fprintf(file, "<binary data of length %u>", val.v.l);
            mpack_skip_bytes(reader, val.v.l);
            mpack_done_bin(reader);
            break;

        case mpack_type_ext:
            fprintf(file, "<ext data of type %i and length %u>", val.exttype, val.v.l);
            mpack_skip_bytes(reader, val.v.l);
            mpack_done_ext(reader);
            break;

        case mpack_type_str:
            putc('"', file);
            for (size_t i = 0; i < val.v.l; ++i) {
                char c;
                mpack_read_bytes(reader, &c, 1);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
                switch (c) {
                    case '\n': fprintf(file, "\\n"); break;
                    case '\\': fprintf(file, "\\\\"); break;
                    case '"': fprintf(file, "\\\""); break;
                    default: putc(c, file); break;
                }
            }
            putc('"', file);
            mpack_done_str(reader);
            break;

        case mpack_type_array:
            fprintf(file, "[\n");
            for (size_t i = 0; i < val.v.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_print_element(reader, depth + 1, file);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
                if (i != val.v.n - 1)
                    putc(',', file);
                putc('\n', file);
            }
            for (size_t i = 0; i < depth; ++i)
                fprintf(file, "    ");
            putc(']', file);
            mpack_done_array(reader);
            break;

        case mpack_type_map:
            fprintf(file, "{\n");
            for (size_t i = 0; i < val.v.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_print_element(reader, depth + 1, file);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
                fprintf(file, ": ");
                mpack_print_element(reader, depth + 1, file);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
                if (i != val.v.n - 1)
                    putc(',', file);
                putc('\n', file);
            }
            for (size_t i = 0; i < depth; ++i)
                fprintf(file, "    ");
            putc('}', file);
            mpack_done_map(reader);
            break;
    }
}

void mpack_print_file(const char* data, size_t len, FILE* file) {
    mpack_assert(data != NULL, "data is NULL");
    mpack_assert(file != NULL, "file is NULL");

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, len);

    size_t depth = 2;
    for (size_t i = 0; i < depth; ++i)
        fprintf(file, "    ");
    mpack_print_element(&reader, depth, file);
    putc('\n', file);

    size_t remaining = mpack_reader_remaining(&reader, NULL);

    if (mpack_reader_destroy(&reader) != mpack_ok)
        fprintf(file, "<mpack parsing error %s>\n", mpack_error_to_string(mpack_reader_error(&reader)));
    else if (remaining > 0)
        fprintf(file, "<%i extra bytes at end of mpack>\n", (int)remaining);
}
#endif

#endif

