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

#include "mpack-reader.h"

#if MPACK_READER

static void mpack_reader_skip_using_fill(mpack_reader_t* reader, size_t count);

void mpack_reader_init(mpack_reader_t* reader, char* buffer, size_t size, size_t count) {
    mpack_assert(buffer != NULL, "buffer is NULL");

    mpack_memset(reader, 0, sizeof(*reader));
    reader->buffer = buffer;
    reader->size = size;
    reader->left = count;
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_init(&reader->track)));

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
    reader->left = count;

    // unfortunately we have to cast away the const to store the buffer,
    // but we won't be modifying it because there's no fill function.
    // the buffer size is left at 0 to ensure no fill function can be
    // set or used (see mpack_reader_set_fill().)
    #ifdef __cplusplus
    reader->buffer = const_cast<char*>(data);
    #else
    reader->buffer = (char*)data;
    #endif

    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_init(&reader->track)));

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
    #if MPACK_OPTIMIZE_FOR_SIZE
    MPACK_UNUSED(reader);
    MPACK_UNUSED(skip);
    #else
    reader->skip = skip;
    #endif
}

#if MPACK_STDIO
typedef struct mpack_file_reader_t {
    FILE* file;
    char buffer[MPACK_BUFFER_SIZE];
} mpack_file_reader_t;

static size_t mpack_file_reader_fill(mpack_reader_t* reader, char* buffer, size_t count) {
    mpack_file_reader_t* file_reader = (mpack_file_reader_t*)reader->context;
    return fread((void*)buffer, 1, count, file_reader->file);
}

#if !MPACK_OPTIMIZE_FOR_SIZE
static void mpack_file_reader_skip(mpack_reader_t* reader, size_t count) {
    mpack_file_reader_t* file_reader = (mpack_file_reader_t*)reader->context;
    if (mpack_reader_error(reader) != mpack_ok)
        return;
    FILE* file = file_reader->file;

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
#endif

static void mpack_file_reader_teardown(mpack_reader_t* reader) {
    mpack_file_reader_t* file_reader = (mpack_file_reader_t*)reader->context;

    if (file_reader->file) {
        int ret = fclose(file_reader->file);
        file_reader->file = NULL;
        if (ret != 0)
            mpack_reader_flag_error(reader, mpack_error_io);
    }

    MPACK_FREE(file_reader);
}

void mpack_reader_init_file(mpack_reader_t* reader, const char* filename) {
    mpack_assert(filename != NULL, "filename is NULL");

    mpack_file_reader_t* file_reader = (mpack_file_reader_t*) MPACK_MALLOC(sizeof(mpack_file_reader_t));
    if (file_reader == NULL) {
        mpack_reader_init_error(reader, mpack_error_memory);
        return;
    }

    file_reader->file = fopen(filename, "rb");
    if (file_reader->file == NULL) {
        mpack_reader_init_error(reader, mpack_error_io);
        MPACK_FREE(file_reader);
        return;
    }

    mpack_reader_init(reader, file_reader->buffer, sizeof(file_reader->buffer), 0);
    mpack_reader_set_context(reader, file_reader);
    mpack_reader_set_fill(reader, mpack_file_reader_fill);
    #if !MPACK_OPTIMIZE_FOR_SIZE
    mpack_reader_set_skip(reader, mpack_file_reader_skip);
    #endif
    mpack_reader_set_teardown(reader, mpack_file_reader_teardown);
}
#endif

mpack_error_t mpack_reader_destroy(mpack_reader_t* reader) {

    // clean up tracking, asserting if we're not already in an error state
    #if MPACK_READ_TRACKING
    mpack_track_destroy(&reader->track, reader->error != mpack_ok);
    #endif

    if (reader->teardown)
        reader->teardown(reader);
    reader->teardown = NULL;

    return reader->error;
}

size_t mpack_reader_remaining(mpack_reader_t* reader, const char** data) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_check_empty(&reader->track)));
    if (data)
        *data = reader->buffer + reader->pos;
    return reader->left;
}

void mpack_reader_flag_error(mpack_reader_t* reader, mpack_error_t error) {
    mpack_log("reader %p setting error %i: %s\n", reader, (int)error, mpack_error_to_string(error));

    if (reader->error == mpack_ok) {
        reader->error = error;
        if (reader->error_fn)
            reader->error_fn(reader, error);
    }
}

// A helper to call the reader fill function. This makes sure it's
// implemented and guards against overflow in case it returns -1.
MPACK_STATIC_INLINE size_t mpack_fill(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(reader->fill != NULL, "mpack_fill() called with no fill function?");

    size_t ret = reader->fill(reader, p, count);
    if (ret == ((size_t)(-1)))
        return 0;

    return ret;
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
        mpack_memcpy(p, reader->buffer + reader->pos, reader->left);
        count -= reader->left;
        p += reader->left;
        reader->pos += reader->left;
        reader->left = 0;
    }

    // we read only in multiples of the buffer size. read the middle portion, if any
    size_t middle = count - (count % reader->size);
    if (middle > 0) {
        mpack_log("reading %i bytes in middle\n", (int)middle);
        if (mpack_fill(reader, p, middle) < middle) {
            mpack_reader_flag_error(reader, mpack_error_io);
            mpack_memset(p, 0, count);
            return;
        }
        count -= middle;
        p += middle;
        if (count == 0)
            return;
    }

    // fill the buffer
    reader->pos = 0;
    reader->left = mpack_fill(reader, reader->buffer, reader->size);
    mpack_log("filled %i bytes into buffer\n", (int)reader->left);
    if (reader->left < count) {
        mpack_reader_flag_error(reader, mpack_error_io);
        mpack_memset(p, 0, count);
        return;
    }

    // serve the remainder
    mpack_log("serving %i remaining bytes from %p to %p\n", (int)count, reader->buffer+reader->pos,p);
    mpack_memcpy(p, reader->buffer + reader->pos, count);
    reader->pos += count;
    reader->left -= count;
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
        reader->pos += count;
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
    reader->pos += reader->left;
    reader->left = 0;

    #if !MPACK_OPTIMIZE_FOR_SIZE
    // use the skip function if we've got one, and if we're trying
    // to skip a lot of data. if we only need to skip some tiny
    // fraction of the buffer size, it's probably better to just
    // fill the buffer and skip from it instead of trying to seek.
    if (reader->skip && count > reader->size / 16) {
        mpack_log("calling skip function for %i bytes\n", (int)count);
        reader->skip(reader, count);
        return;
    }
    #endif

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
        mpack_fill(reader, reader->buffer, reader->size);
        if (mpack_fill(reader, reader->buffer, reader->size) < reader->size) {
            mpack_reader_flag_error(reader, mpack_error_io);
            return;
        }
        count -= reader->size;
    }

    // fill the buffer as much as possible
    reader->pos = 0;
    reader->left = mpack_fill(reader, reader->buffer, reader->size);
    if (reader->left < count)
        mpack_reader_flag_error(reader, mpack_error_io);
    mpack_log("filled %i bytes into buffer; discarding %i bytes\n", (int)reader->left, (int)count);
    reader->pos += count;
    reader->left -= count;
}

void mpack_read_bytes(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(p != NULL, "destination for read of %i bytes is NULL", (int)count);
    mpack_reader_track_bytes(reader, count);
    mpack_read_native(reader, p, count);
}

#ifdef MPACK_MALLOC
// Reads native bytes with error callback disabled. This allows MPack reader functions
// to hold an allocated buffer and read native data into it without leaking it in
// case of a non-local jump out of an error handler.
static void mpack_read_native_nojump(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(reader->error == mpack_ok, "cannot call nojump if an error is already flagged!");
    mpack_reader_error_t error_fn = reader->error_fn;
    reader->error_fn = NULL;
    mpack_read_native(reader, p, count);
    reader->error_fn = error_fn;
}

char* mpack_read_bytes_alloc_size(mpack_reader_t* reader, size_t count, size_t alloc_size) {
    mpack_assert(count <= alloc_size, "count %i is less than alloc_size %i", (int)count, (int)alloc_size);
    if (alloc_size == 0)
        return NULL;

    // track the bytes first in case it jumps
    mpack_reader_track_bytes(reader, count);
    if (mpack_reader_error(reader) != mpack_ok)
        return NULL;

    // allocate data
    char* data = (char*)MPACK_MALLOC(alloc_size);
    if (data == NULL) {
        mpack_reader_flag_error(reader, mpack_error_memory);
        return NULL;
    }

    // read with jump disabled so we don't leak our buffer
    mpack_read_native_nojump(reader, data, count);

    // report flagged errors
    if (mpack_reader_error(reader) != mpack_ok) {
        MPACK_FREE(data);
        if (reader->error_fn)
            reader->error_fn(reader, mpack_reader_error(reader));
        return NULL;
    }

    return data;
}
#endif

// Fills the buffer when there is already some data in the buffer. The
// existing data is moved to the start of the buffer.
MPACK_STATIC_INLINE void mpack_partial_fill(mpack_reader_t* reader) {
    mpack_memmove(reader->buffer, reader->buffer + reader->pos, reader->left);
    reader->pos = 0;
    reader->left += mpack_fill(reader, reader->buffer + reader->left, reader->size - reader->left);
}

// internal inplace reader for when it straddles the end of the buffer
// this is split out to inline the common case, although this isn't done
// right now because we can't inline tracking yet
static const char* mpack_read_bytes_inplace_big(mpack_reader_t* reader, size_t count) {

    // we should only arrive here from inplace straddle; this should already be checked
    mpack_assert(mpack_reader_error(reader) == mpack_ok, "already in error state? %s",
            mpack_error_to_string(mpack_reader_error(reader)));
    mpack_assert(reader->left < count, "already enough bytes in buffer: %i left, %i count", (int)reader->left, (int)count);

    // we'll need a fill function to get more data. if there's no
    // fill function, the buffer should contain an entire MessagePack
    // object, so we raise mpack_error_invalid instead of mpack_error_io
    // on truncated data.
    if (reader->fill == NULL) {
        mpack_reader_flag_error(reader, mpack_error_invalid);
        return NULL;
    }

    // make sure the buffer is big enough to actually fit the data
    if (count > reader->size) {
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return NULL;
    }

    // re-fill as much as possible
    mpack_partial_fill(reader);

    // shift the remaining data back to the start and fill the buffer back up
    if (reader->left < count) {
        mpack_reader_flag_error(reader, mpack_error_io);
        return NULL;
    }
    reader->pos += count;
    reader->left -= count;
    return reader->buffer;
}

const char* mpack_read_bytes_inplace(mpack_reader_t* reader, size_t count) {
    if (mpack_reader_error(reader) != mpack_ok)
        return NULL;

    mpack_reader_track_bytes(reader, count);

    // if we have enough bytes already in the buffer, we can return it directly.
    if (reader->left >= count) {
        reader->pos += count;
        reader->left -= count;
        return reader->buffer + reader->pos - count;
    }

    return mpack_read_bytes_inplace_big(reader, count);
}

// Decodes a tag from a byte buffer. The size of the bytes buffer
// must be at least MPACK_MINIMUM_TAG_SIZE.
static size_t mpack_decode_tag(const char* bytes, mpack_tag_t* tag) {
    uint8_t type = mpack_load_native_u8(bytes++);

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
            tag->v.n = type & ~0xf0;
            return 1;

        // fixarray
        case 0x9:
            tag->type = mpack_type_array;
            tag->v.n = type & ~0xf0;
            return 1;

        // fixstr
        case 0xa: case 0xb:
            tag->type = mpack_type_str;
            tag->v.l = type & ~0xe0;
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
            tag->v.n = type & ~0xf0;
            return 1;

        // fixarray
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            tag->type = mpack_type_array;
            tag->v.n = type & ~0xf0;
            return 1;

        // fixstr
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            tag->type = mpack_type_str;
            tag->v.l = type & ~0xe0;
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
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_native_u8(bytes);
            return 2;

        // bin16
        case 0xc5:
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_native_u16(bytes);
            return 3;

        // bin32
        case 0xc6:
            tag->type = mpack_type_bin;
            tag->v.l = mpack_load_native_u32(bytes);
            return 5;

        // ext8
        case 0xc7:
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_native_u8(bytes);
            tag->exttype = mpack_load_native_i8(bytes + 1);
            return 3;

        // ext16
        case 0xc8:
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_native_u16(bytes);
            tag->exttype = mpack_load_native_i8(bytes + 2);
            return 4;

        // ext32
        case 0xc9:
            tag->type = mpack_type_ext;
            tag->v.l = mpack_load_native_u32(bytes);
            tag->exttype = mpack_load_native_i8(bytes + 4);
            return 6;

        // float
        case 0xca:
            tag->type = mpack_type_float;
            tag->v.f = mpack_load_native_float(bytes);
            return 5;

        // double
        case 0xcb:
            tag->type = mpack_type_double;
            tag->v.d = mpack_load_native_double(bytes);
            return 9;

        // uint8
        case 0xcc:
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_native_u8(bytes);
            return 2;

        // uint16
        case 0xcd:
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_native_u16(bytes);
            return 3;

        // uint32
        case 0xce:
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_native_u32(bytes);
            return 5;

        // uint64
        case 0xcf:
            tag->type = mpack_type_uint;
            tag->v.u = mpack_load_native_u64(bytes);
            return 9;

        // int8
        case 0xd0:
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_native_i8(bytes);
            return 2;

        // int16
        case 0xd1:
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_native_i16(bytes);
            return 3;

        // int32
        case 0xd2:
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_native_i32(bytes);
            return 5;

        // int64
        case 0xd3:
            tag->type = mpack_type_int;
            tag->v.i = mpack_load_native_i64(bytes);
            return 9;

        // fixext1
        case 0xd4:
            tag->type = mpack_type_ext;
            tag->v.l = 1;
            tag->exttype = mpack_load_native_i8(bytes);
            return 2;

        // fixext2
        case 0xd5:
            tag->type = mpack_type_ext;
            tag->v.l = 2;
            tag->exttype = mpack_load_native_i8(bytes);
            return 2;

        // fixext4
        case 0xd6:
            tag->type = mpack_type_ext;
            tag->v.l = 4;
            tag->exttype = mpack_load_native_i8(bytes);
            return 2;

        // fixext8
        case 0xd7:
            tag->type = mpack_type_ext;
            tag->v.l = 8;
            tag->exttype = mpack_load_native_i8(bytes);
            return 2;

        // fixext16
        case 0xd8:
            tag->type = mpack_type_ext;
            tag->v.l = 16;
            tag->exttype = mpack_load_native_i8(bytes);
            return 2;

        // str8
        case 0xd9:
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_native_u8(bytes);
            return 2;

        // str16
        case 0xda:
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_native_u16(bytes);
            return 3;

        // str32
        case 0xdb:
            tag->type = mpack_type_str;
            tag->v.l = mpack_load_native_u32(bytes);
            return 5;

        // array16
        case 0xdc:
            tag->type = mpack_type_array;
            tag->v.n = mpack_load_native_u16(bytes);
            return 3;

        // array32
        case 0xdd:
            tag->type = mpack_type_array;
            tag->v.n = mpack_load_native_u32(bytes);
            return 5;

        // map16
        case 0xde:
            tag->type = mpack_type_map;
            tag->v.n = mpack_load_native_u16(bytes);
            return 3;

        // map32
        case 0xdf:
            tag->type = mpack_type_map;
            tag->v.n = mpack_load_native_u32(bytes);
            return 5;

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
    return 0;
}

MPACK_STATIC_INLINE size_t mpack_parse_tag_in_place(mpack_reader_t* reader, mpack_tag_t* tag) {
    mpack_log("decoding tag in-place, %i left\n", (int)reader->left);
    size_t count = mpack_decode_tag(reader->buffer + reader->pos, tag);
    mpack_log("tag took %i bytes\n", (int)count);
    if (count == 0)
        mpack_reader_flag_error(reader, mpack_error_invalid);
    return count;
}

// Parses a tag from the reader, returning its size in the reader buffer.
// Returns 0 and flags an error if an error occurs.
static size_t mpack_parse_tag(mpack_reader_t* reader, mpack_tag_t* tag) {

    // if there is enough data in the buffer to parse the largest
    // possible tag, we can decode the tag in-place straight from
    // the buffer. this is by far the most common case.
    if (reader->left >= MPACK_MAXIMUM_TAG_SIZE)
        return mpack_parse_tag_in_place(reader, tag);

    // if we have a fill function, we refill the buffer and try
    // to decode in-place again. (it's faster not to do the
    // refill check first)
    if (reader->fill && reader->left < MPACK_MAXIMUM_TAG_SIZE) {
        mpack_log("only %i bytes left to read tag, trying to refill\n", (int)reader->left);
        mpack_partial_fill(reader);
        if (reader->left >= MPACK_MAXIMUM_TAG_SIZE)
            return mpack_parse_tag_in_place(reader, tag);
    }

    // if there is not enough data left in the buffer for the largest
    // possible tag, we copy the remaining data into a large enough
    // temporary and zero out the extra space. we can then parse any
    // tag from it and flag an error if more bytes were used from the
    // temporary than are available in the original data.
    // (technically, zeroing out the extra data is not necessary because
    // we're going to discard it if it's used, but valgrind doesn't
    // know that so it's better to just zero it so it doesn't complain.)
    char buf[MPACK_MAXIMUM_TAG_SIZE];
    mpack_memcpy(buf, reader->buffer + reader->pos, reader->left);
    mpack_memset(buf + reader->left, 0, sizeof(buf) - reader->left);
    mpack_log("decoding tag from local buffer, %i left\n", (int)reader->left);
    size_t count = mpack_decode_tag(buf, tag);
    mpack_log("tag took %i bytes, %i left\n", (int)count, (int)reader->left);

    // if we have a fill function and didn't have enough data, it's an io error
    if (count > reader->left && reader->fill != NULL) {
        mpack_reader_flag_error(reader, mpack_error_io);
        return 0;
    }

    // otherwise, as in mpack_read_native_big(), we're invalid
    if (count == 0 || count > reader->left) {
        mpack_reader_flag_error(reader, mpack_error_invalid);
        return 0;
    }

    return count;
}

mpack_tag_t mpack_read_tag(mpack_reader_t* reader) {
    mpack_log("reading tag\n");

    // make sure we can read a tag
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

    // the tag is guaranteed to have been read out of
    // the buffer, so we advance past it
    reader->pos += count;
    reader->left -= count;

    return tag;
}

mpack_tag_t mpack_peek_tag(mpack_reader_t* reader) {
    mpack_log("peeking tag\n");

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
void mpack_done_array(mpack_reader_t* reader) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, mpack_type_array)));
}

void mpack_done_map(mpack_reader_t* reader) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, mpack_type_map)));
}

void mpack_done_str(mpack_reader_t* reader) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, mpack_type_str)));
}

void mpack_done_bin(mpack_reader_t* reader) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, mpack_type_bin)));
}

void mpack_done_ext(mpack_reader_t* reader) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, mpack_type_ext)));
}

void mpack_done_type(mpack_reader_t* reader, mpack_type_t type) {
    MPACK_UNUSED(MPACK_READER_TRACK(reader, mpack_track_pop(&reader->track, type)));
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

    int depth = 2;
    for (int i = 0; i < depth; ++i)
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

