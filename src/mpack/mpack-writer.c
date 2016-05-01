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

#include "mpack-writer.h"

#if MPACK_WRITER

#if MPACK_WRITE_TRACKING
static void mpack_writer_flag_if_error(mpack_writer_t* writer, mpack_error_t error) {
    if (error != mpack_ok)
        mpack_writer_flag_error(writer, error);
}

void mpack_writer_track_push(mpack_writer_t* writer, mpack_type_t type, uint64_t count) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_push(&writer->track, type, count));
}

void mpack_writer_track_pop(mpack_writer_t* writer, mpack_type_t type) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_pop(&writer->track, type));
}

void mpack_writer_track_element(mpack_writer_t* writer) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_element(&writer->track, false));
}

void mpack_writer_track_bytes(mpack_writer_t* writer, size_t count) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_bytes(&writer->track, false, count));
}
#endif

static void mpack_writer_clear(mpack_writer_t* writer) {
    #if MPACK_COMPATIBILITY
    writer->version = mpack_version_current;
    #endif
    writer->flush = NULL;
    writer->error_fn = NULL;
    writer->teardown = NULL;
    writer->context = NULL;

    writer->buffer = NULL;
    writer->size = 0;
    writer->used = 0;
    writer->error = mpack_ok;

    #if MPACK_WRITE_TRACKING
    mpack_memset(&writer->track, 0, sizeof(writer->track));
    #endif
}

void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size) {
    mpack_assert(buffer != NULL, "cannot initialize writer with empty buffer");
    mpack_writer_clear(writer);
    writer->buffer = buffer;
    writer->size = size;

    #if MPACK_WRITE_TRACKING
    mpack_writer_flag_if_error(writer, mpack_track_init(&writer->track));
    #endif

    mpack_log("===========================\n");
    mpack_log("initializing writer with buffer size %i\n", (int)size);
}

void mpack_writer_init_error(mpack_writer_t* writer, mpack_error_t error) {
    mpack_writer_clear(writer);
    writer->error = error;

    mpack_log("===========================\n");
    mpack_log("initializing writer in error state %i\n", (int)error);
}

void mpack_writer_set_flush(mpack_writer_t* writer, mpack_writer_flush_t flush) {
    MPACK_STATIC_ASSERT(MPACK_WRITER_MINIMUM_BUFFER_SIZE >= MPACK_MAXIMUM_TAG_SIZE,
            "minimum buffer size must fit any tag!");

    if (writer->size < MPACK_WRITER_MINIMUM_BUFFER_SIZE) {
        mpack_break("buffer size is %i, but minimum buffer size for flush is %i",
                (int)writer->size, MPACK_WRITER_MINIMUM_BUFFER_SIZE);
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    writer->flush = flush;
}

#ifdef MPACK_MALLOC
typedef struct mpack_growable_writer_t {
    char** target_data;
    size_t* target_size;
} mpack_growable_writer_t;

static void mpack_growable_writer_flush(mpack_writer_t* writer, const char* data, size_t count) {

    // This is an intrusive flush function which modifies the writer's buffer
    // in response to a flush instead of emptying it in order to add more
    // capacity for data. This removes the need to copy data from a fixed buffer
    // into a growable one, improving performance.
    //
    // There are three ways flush can be called:
    //   - flushing the buffer during writing (used is zero, count is all data, data is buffer)
    //   - flushing extra data during writing (used is all flushed data, count is extra data, data is not buffer)
    //   - flushing during teardown (used and count are both all flushed data, data is buffer)
    //
    // In the first two cases, we grow the buffer by at least double, enough
    // to ensure that new data will fit. We ignore the teardown flush.

    if (data == writer->buffer) {

        // teardown, do nothing
        if (writer->used == count)
            return;

        // otherwise leave the data in the buffer and just grow
        writer->used = count;
        count = 0;
    }

    mpack_log("flush size %i used %i data %p buffer %p\n",
            (int)count, (int)writer->used, data, writer->buffer);

    mpack_assert(data == writer->buffer || writer->used + count > writer->size,
            "extra flush for %i but there is %i space left in the buffer! (%i/%i)",
            (int)count, (int)writer->size - (int)writer->used, (int)writer->used, (int)writer->size);

    // grow to fit the data
    // TODO: this really needs to correctly test for overflow
    size_t new_size = writer->size * 2;
    while (new_size < writer->used + count)
        new_size *= 2;

    mpack_log("flush growing buffer size from %i to %i\n", (int)writer->size, (int)new_size);

    // grow the buffer
    char* new_buffer = (char*)mpack_realloc(writer->buffer, writer->used, new_size);
    if (new_buffer == NULL) {
        mpack_writer_flag_error(writer, mpack_error_memory);
        return;
    }
    writer->buffer = new_buffer;
    writer->size = new_size;

    // append the extra data
    if (count > 0) {
        mpack_memcpy(writer->buffer + writer->used, data, count);
        writer->used += count;
    }

    mpack_log("new buffer %p, used %i\n", new_buffer, (int)writer->used);
}

static void mpack_growable_writer_teardown(mpack_writer_t* writer) {
    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*)writer->context;

    if (mpack_writer_error(writer) == mpack_ok) {

        // shrink the buffer to an appropriate size if the data is
        // much smaller than the buffer
        if (writer->used < writer->size / 2) {
            char* buffer = (char*)mpack_realloc(writer->buffer, writer->used, writer->used);
            if (!buffer) {
                MPACK_FREE(writer->buffer);
                mpack_writer_flag_error(writer, mpack_error_memory);
                return;
            }
            writer->buffer = buffer;
            writer->size = writer->used;
        }

        *growable_writer->target_data = writer->buffer;
        *growable_writer->target_size = writer->used;
        writer->buffer = NULL;

    } else if (writer->buffer) {
        MPACK_FREE(writer->buffer);
        writer->buffer = NULL;
    }

    writer->context = NULL;
}

static char* mpack_writer_get_reserved(mpack_writer_t* writer) {
    // This is in a separate function in order to avoid false strict aliasing
    // warnings. We aren't actually violating strict aliasing (the reserved
    // space is only ever dereferenced as an mpack_growable_writer_t.)
    return (char*)writer->reserved;
}

void mpack_writer_init_growable(mpack_writer_t* writer, char** target_data, size_t* target_size) {
    mpack_assert(target_data != NULL, "cannot initialize writer without a destination for the data");
    mpack_assert(target_size != NULL, "cannot initialize writer without a destination for the size");

    *target_data = NULL;
    *target_size = 0;

    MPACK_STATIC_ASSERT(sizeof(mpack_growable_writer_t) <= sizeof(writer->reserved),
            "not enough reserved space for growable writer!");
    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*)mpack_writer_get_reserved(writer);

    growable_writer->target_data = target_data;
    growable_writer->target_size = target_size;

    size_t capacity = MPACK_BUFFER_SIZE;
    char* buffer = (char*)MPACK_MALLOC(capacity);
    if (buffer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        return;
    }

    mpack_writer_init(writer, buffer, capacity);
    mpack_writer_set_context(writer, growable_writer);
    mpack_writer_set_flush(writer, mpack_growable_writer_flush);
    mpack_writer_set_teardown(writer, mpack_growable_writer_teardown);
}
#endif

#if MPACK_STDIO
static void mpack_file_writer_flush(mpack_writer_t* writer, const char* buffer, size_t count) {
    FILE* file = (FILE*)writer->context;
    size_t written = fwrite((const void*)buffer, 1, count, file);
    if (written != count)
        mpack_writer_flag_error(writer, mpack_error_io);
}

static void mpack_file_writer_teardown(mpack_writer_t* writer) {
    FILE* file = (FILE*)writer->context;

    if (file) {
        int ret = fclose(file);
        writer->context = NULL;
        if (ret != 0)
            mpack_writer_flag_error(writer, mpack_error_io);
    }

    MPACK_FREE(writer->buffer);
    writer->buffer = NULL;
}

void mpack_writer_init_file(mpack_writer_t* writer, const char* filename) {
    mpack_assert(filename != NULL, "filename is NULL");

    size_t capacity = MPACK_BUFFER_SIZE;
    char* buffer = (char*)MPACK_MALLOC(capacity);
    if (buffer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        return;
    }

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        MPACK_FREE(buffer);
        mpack_writer_init_error(writer, mpack_error_io);
        return;
    }

    mpack_writer_init(writer, buffer, capacity);
    mpack_writer_set_context(writer, file);
    mpack_writer_set_flush(writer, mpack_file_writer_flush);
    mpack_writer_set_teardown(writer, mpack_file_writer_teardown);
}
#endif

void mpack_writer_flag_error(mpack_writer_t* writer, mpack_error_t error) {
    mpack_log("writer %p setting error %i: %s\n", writer, (int)error, mpack_error_to_string(error));

    if (writer->error == mpack_ok) {
        writer->error = error;
        if (writer->error_fn)
            writer->error_fn(writer, writer->error);
    }
}

MPACK_STATIC_INLINE void mpack_writer_flush_unchecked(mpack_writer_t* writer) {
    // This is a bit ugly; we reset used before calling flush so that
    // a flush function can distinguish between flushing the buffer
    // versus flushing external data. see mpack_growable_writer_flush()
    size_t used = writer->used;
    writer->used = 0;
    writer->flush(writer, writer->buffer, used);
}

void mpack_writer_flush_message(mpack_writer_t* writer) {
    if (writer->error != mpack_ok)
        return;

    #if MPACK_WRITE_TRACKING
    mpack_writer_flag_if_error(writer, mpack_track_check_empty(&writer->track));
    if (writer->error != mpack_ok)
        return;
    #endif

    if (writer->flush == NULL) {
        mpack_break("cannot call mpack_writer_flush_message() without a flush function!");
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    if (writer->used > 0)
        mpack_writer_flush_unchecked(writer);
}

// Ensures there are at least count bytes free in the buffer. This
// will flag an error if the flush function fails to make enough
// room in the buffer.
static bool mpack_writer_ensure(mpack_writer_t* writer, size_t count) {
    mpack_assert(count != 0, "cannot ensure zero bytes!");
    mpack_assert(count <= MPACK_WRITER_MINIMUM_BUFFER_SIZE,
            "cannot ensure %i bytes, this is more than the minimum buffer size %i!",
            (int)count, (int)MPACK_WRITER_MINIMUM_BUFFER_SIZE);
    mpack_assert(count > mpack_writer_buffer_left(writer),
            "request to ensure %i bytes but there are already %i left in the buffer!",
            (int)count, (int)mpack_writer_buffer_left(writer));

    mpack_log("ensuring %i bytes, %i left\n", (int)count, (int)mpack_writer_buffer_left(writer));

    if (mpack_writer_error(writer) != mpack_ok)
        return false;

    if (writer->flush == NULL) {
        mpack_writer_flag_error(writer, mpack_error_too_big);
        return false;
    }

    mpack_writer_flush_unchecked(writer);
    if (mpack_writer_error(writer) != mpack_ok)
        return false;

    if (mpack_writer_buffer_left(writer) >= count)
        return true;

    mpack_writer_flag_error(writer, mpack_error_io);
    return false;
}

// Writes encoded bytes to the buffer when we already know the data
// does not fit in the buffer (i.e. it straddles the edge of the
// buffer.) If there is a flush function, it is guaranteed to be
// called; otherwise mpack_error_too_big is raised.
static void mpack_write_native_straddle(mpack_writer_t* writer, const char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (mpack_writer_error(writer) != mpack_ok)
        return;
    mpack_log("big write for %i bytes from %p, %i space left in buffer\n",
            (int)count, p, (int)(writer->size - writer->used));
    mpack_assert(count > writer->size - writer->used,
            "big write requested for %i bytes, but there is %i available "
            "space in buffer. should have called mpack_write_native() instead",
            (int)count, (int)(writer->size - writer->used));

    // we'll need a flush function
    if (!writer->flush) {
        mpack_writer_flag_error(writer, mpack_error_too_big);
        return;
    }

    // flush the buffer
    mpack_writer_flush_unchecked(writer);
    if (mpack_writer_error(writer) != mpack_ok)
        return;

    // note that an intrusive flush function (such as mpack_growable_writer_flush())
    // may have changed size and/or reset used to a non-zero value. we treat both as
    // though they may have changed, and there may still be data in the buffer.

    // flush the extra data directly if it doesn't fit in the buffer
    if (count > writer->size - writer->used) {
        writer->flush(writer, p, count);
        if (mpack_writer_error(writer) != mpack_ok)
            return;
    } else {
        mpack_memcpy(writer->buffer + writer->used, p, count);
        writer->used += count;
    }
}

// Writes encoded bytes to the buffer, flushing if necessary.
MPACK_STATIC_INLINE void mpack_write_native(mpack_writer_t* writer, const char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (writer->size - writer->used < count) {
        mpack_write_native_straddle(writer, p, count);
    } else {
        mpack_memcpy(writer->buffer + writer->used, p, count);
        writer->used += count;
    }
}

mpack_error_t mpack_writer_destroy(mpack_writer_t* writer) {

    // clean up tracking, asserting if we're not already in an error state
    #if MPACK_WRITE_TRACKING
    mpack_track_destroy(&writer->track, writer->error != mpack_ok);
    #endif

    // flush any outstanding data
    if (mpack_writer_error(writer) == mpack_ok && writer->used != 0 && writer->flush != NULL) {
        writer->flush(writer, writer->buffer, writer->used);
        writer->flush = NULL;
    }

    if (writer->teardown) {
        writer->teardown(writer);
        writer->teardown = NULL;
    }

    return writer->error;
}

void mpack_write_tag(mpack_writer_t* writer, mpack_tag_t value) {
    switch (value.type) {
        case mpack_type_nil:    mpack_write_nil   (writer);            break;
        case mpack_type_bool:   mpack_write_bool  (writer, value.v.b); break;
        case mpack_type_float:  mpack_write_float (writer, value.v.f); break;
        case mpack_type_double: mpack_write_double(writer, value.v.d); break;
        case mpack_type_int:    mpack_write_int   (writer, value.v.i); break;
        case mpack_type_uint:   mpack_write_uint  (writer, value.v.u); break;

        case mpack_type_str: mpack_start_str(writer, value.v.l); break;
        case mpack_type_bin: mpack_start_bin(writer, value.v.l); break;
        case mpack_type_ext: mpack_start_ext(writer, value.exttype, value.v.l); break;

        case mpack_type_array: mpack_start_array(writer, value.v.n); break;
        case mpack_type_map:   mpack_start_map(writer, value.v.n);   break;

        default:
            mpack_assert(0, "unrecognized type %i", (int)value.type);
            break;
    }
}

MPACK_STATIC_INLINE void mpack_write_byte_element(mpack_writer_t* writer, char value) {
    mpack_writer_track_element(writer);
    if (mpack_writer_buffer_left(writer) >= 1 || mpack_writer_ensure(writer, 1))
        writer->buffer[writer->used++] = value;
}

void mpack_write_nil(mpack_writer_t* writer) {
    mpack_write_byte_element(writer, (char)0xc0);
}

void mpack_write_bool(mpack_writer_t* writer, bool value) {
    mpack_write_byte_element(writer, (char)(0xc2 | (value ? 1 : 0)));
}

void mpack_write_true(mpack_writer_t* writer) {
    mpack_write_byte_element(writer, (char)0xc3);
}

void mpack_write_false(mpack_writer_t* writer) {
    mpack_write_byte_element(writer, (char)0xc2);
}

void mpack_write_object_bytes(mpack_writer_t* writer, const char* data, size_t bytes) {
    mpack_writer_track_element(writer);
    mpack_write_native(writer, data, bytes);
}

/*
 * Encode functions
 */

MPACK_STATIC_INLINE void mpack_encode_fixuint(char* p, uint8_t value) {
    mpack_assert(value <= 127);
    mpack_store_u8(p, value);
}

MPACK_STATIC_INLINE void mpack_encode_u8(char* p, uint8_t value) {
    mpack_assert(value > 127);
    mpack_store_u8(p, 0xcc);
    mpack_store_u8(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_u16(char* p, uint16_t value) {
    mpack_assert(value > UINT8_MAX);
    mpack_store_u8(p, 0xcd);
    mpack_store_u16(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_u32(char* p, uint32_t value) {
    mpack_assert(value > UINT16_MAX);
    mpack_store_u8(p, 0xce);
    mpack_store_u32(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_u64(char* p, uint64_t value) {
    mpack_assert(value > UINT32_MAX);
    mpack_store_u8(p, 0xcf);
    mpack_store_u64(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_fixint(char* p, int8_t value) {
    // this can encode positive or negative fixints
    mpack_assert(value >= -32);
    mpack_store_i8(p, value);
}

MPACK_STATIC_INLINE void mpack_encode_i8(char* p, int8_t value) {
    mpack_assert(value < -32);
    mpack_store_u8(p, 0xd0);
    mpack_store_i8(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_i16(char* p, int16_t value) {
    mpack_assert(value < INT8_MIN);
    mpack_store_u8(p, 0xd1);
    mpack_store_i16(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_i32(char* p, int32_t value) {
    mpack_assert(value < INT16_MIN);
    mpack_store_u8(p, 0xd2);
    mpack_store_i32(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_i64(char* p, int64_t value) {
    mpack_assert(value < INT32_MIN);
    mpack_store_u8(p, 0xd3);
    mpack_store_i64(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_float(char* p, float value) {
    mpack_store_u8(p, 0xca);
    mpack_store_float(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_double(char* p, double value) {
    mpack_store_u8(p, 0xcb);
    mpack_store_double(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_fixarray(char* p, uint8_t count) {
    mpack_assert(count <= 15);
    mpack_store_u8(p, (uint8_t)(0x90 | count));
}

MPACK_STATIC_INLINE void mpack_encode_array16(char* p, uint16_t count) {
    mpack_assert(count > 15);
    mpack_store_u8(p, 0xdc);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_array32(char* p, uint32_t count) {
    mpack_assert(count > UINT16_MAX);
    mpack_store_u8(p, 0xdd);
    mpack_store_u32(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_fixmap(char* p, uint8_t count) {
    mpack_assert(count <= 15);
    mpack_store_u8(p, (uint8_t)(0x80 | count));
}

MPACK_STATIC_INLINE void mpack_encode_map16(char* p, uint16_t count) {
    mpack_assert(count > 15);
    mpack_store_u8(p, 0xde);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_map32(char* p, uint32_t count) {
    mpack_assert(count > UINT16_MAX);
    mpack_store_u8(p, 0xdf);
    mpack_store_u32(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_fixstr(char* p, uint8_t count) {
    mpack_assert(count <= 31);
    mpack_store_u8(p, (uint8_t)(0xa0 | count));
}

MPACK_STATIC_INLINE void mpack_encode_str8(char* p, uint8_t count) {
    mpack_assert(count > 31);
    mpack_store_u8(p, 0xd9);
    mpack_store_u8(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_str16(char* p, uint16_t count) {
    // we might be encoding a raw in compatibility mode, so we
    // allow count to be in the range [32, UINT8_MAX].
    mpack_assert(count > 31);
    mpack_store_u8(p, 0xda);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_str32(char* p, uint32_t count) {
    mpack_assert(count > UINT16_MAX);
    mpack_store_u8(p, 0xdb);
    mpack_store_u32(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin8(char* p, uint8_t count) {
    mpack_store_u8(p, 0xc4);
    mpack_store_u8(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin16(char* p, uint16_t count) {
    mpack_assert(count > UINT8_MAX);
    mpack_store_u8(p, 0xc5);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin32(char* p, uint32_t count) {
    mpack_assert(count > UINT16_MAX);
    mpack_store_u8(p, 0xc6);
    mpack_store_u32(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_fixext1(char* p, int8_t exttype) {
    mpack_store_u8(p, 0xd4);
    mpack_store_i8(p + 1, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_fixext2(char* p, int8_t exttype) {
    mpack_store_u8(p, 0xd5);
    mpack_store_i8(p + 1, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_fixext4(char* p, int8_t exttype) {
    mpack_store_u8(p, 0xd6);
    mpack_store_i8(p + 1, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_fixext8(char* p, int8_t exttype) {
    mpack_store_u8(p, 0xd7);
    mpack_store_i8(p + 1, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_fixext16(char* p, int8_t exttype) {
    mpack_store_u8(p, 0xd8);
    mpack_store_i8(p + 1, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_ext8(char* p, int8_t exttype, uint8_t count) {
    mpack_assert(count != 1 && count != 2 && count != 4 && count != 8 && count != 16);
    mpack_store_u8(p, 0xc7);
    mpack_store_u8(p + 1, count);
    mpack_store_i8(p + 2, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_ext16(char* p, int8_t exttype, uint16_t count) {
    mpack_assert(count > UINT8_MAX);
    mpack_store_u8(p, 0xc8);
    mpack_store_u16(p + 1, count);
    mpack_store_i8(p + 3, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_ext32(char* p, int8_t exttype, uint32_t count) {
    mpack_assert(count > UINT16_MAX);
    mpack_store_u8(p, 0xc9);
    mpack_store_u32(p + 1, count);
    mpack_store_i8(p + 5, exttype);
}



/*
 * Write functions
 */

// This is a macro wrapper to the encode functions to encode
// directly into the buffer. If mpack_writer_ensure() fails
// it will flag an error so we don't have to do anything.
#define MPACK_WRITE_ENCODED(encode_fn, size, ...) do {                                    \
    if (mpack_writer_buffer_left(writer) >= size || mpack_writer_ensure(writer, size)) {  \
        encode_fn(writer->buffer + writer->used, __VA_ARGS__);                            \
        writer->used += size;                                                             \
    }                                                                                     \
} while (0)

void mpack_write_u8(mpack_writer_t* writer, uint8_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_u64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value <= 127) {
        MPACK_WRITE_ENCODED(mpack_encode_fixuint, MPACK_TAG_SIZE_FIXUINT, value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, value);
    }
    #endif
}

void mpack_write_u16(mpack_writer_t* writer, uint16_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_u64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value <= 127) {
        MPACK_WRITE_ENCODED(mpack_encode_fixuint, MPACK_TAG_SIZE_FIXUINT, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, value);
    }
    #endif
}

void mpack_write_u32(mpack_writer_t* writer, uint32_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_u64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value <= 127) {
        MPACK_WRITE_ENCODED(mpack_encode_fixuint, MPACK_TAG_SIZE_FIXUINT, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
    } else if (value <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, value);
    }
    #endif
}

void mpack_write_u64(mpack_writer_t* writer, uint64_t value) {
    mpack_writer_track_element(writer);

    if (value <= 127) {
        MPACK_WRITE_ENCODED(mpack_encode_fixuint, MPACK_TAG_SIZE_FIXUINT, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
    } else if (value <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
    } else if (value <= UINT32_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, (uint32_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_u64, MPACK_TAG_SIZE_U64, value);
    }
}

void mpack_write_i8(mpack_writer_t* writer, int8_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_i64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value >= -32) {
        // we encode positive and negative fixints together
        MPACK_WRITE_ENCODED(mpack_encode_fixint, MPACK_TAG_SIZE_FIXINT, (int8_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    }
    #endif
}

void mpack_write_i16(mpack_writer_t* writer, int16_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_i64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value >= -32) {
        if (value <= 127) {
            // we encode positive and negative fixints together
            MPACK_WRITE_ENCODED(mpack_encode_fixint, MPACK_TAG_SIZE_FIXINT, (int8_t)value);
        } else if (value <= UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        }
    } else if (value >= INT8_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_i16, MPACK_TAG_SIZE_I16, (int16_t)value);
    }
    #endif
}

void mpack_write_i32(mpack_writer_t* writer, int32_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_write_i64(writer, value);
    #else
    mpack_writer_track_element(writer);
    if (value >= -32) {
        if (value <= 127) {
            // we encode positive and negative fixints together
            MPACK_WRITE_ENCODED(mpack_encode_fixint, MPACK_TAG_SIZE_FIXINT, (int8_t)value);
        } else if (value <= UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else if (value <= UINT16_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, (uint32_t)value);
        }
    } else if (value >= INT8_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    } else if (value >= INT16_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i16, MPACK_TAG_SIZE_I16, (int16_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_i32, MPACK_TAG_SIZE_I32, value);
    }
    #endif
}

void mpack_write_i64(mpack_writer_t* writer, int64_t value) {
    #if MPACK_OPTIMIZE_FOR_SIZE
    if (value > 127) {
        // for non-fix positive ints we call the u64 writer to save space
        mpack_write_u64(writer, (uint64_t)value);
        return;
    }
    #endif

    mpack_writer_track_element(writer);
    if (value >= -32) {
        #if MPACK_OPTIMIZE_FOR_SIZE
        MPACK_WRITE_ENCODED(mpack_encode_fixint, MPACK_TAG_SIZE_FIXINT, (int8_t)value);
        #else
        if (value <= 127) {
            MPACK_WRITE_ENCODED(mpack_encode_fixint, MPACK_TAG_SIZE_FIXINT, (int8_t)value);
        } else if (value <= UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else if (value <= UINT16_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        } else if (value <= UINT32_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, (uint32_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u64, MPACK_TAG_SIZE_U64, (uint64_t)value);
        }
        #endif
    } else if (value >= INT8_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    } else if (value >= INT16_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i16, MPACK_TAG_SIZE_I16, (int16_t)value);
    } else if (value >= INT32_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i32, MPACK_TAG_SIZE_I32, (int32_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_i64, MPACK_TAG_SIZE_I64, value);
    }
}

void mpack_write_float(mpack_writer_t* writer, float value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_float, MPACK_TAG_SIZE_FLOAT, value);
}
void mpack_write_double(mpack_writer_t* writer, double value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_double, MPACK_TAG_SIZE_DOUBLE, value);
}

void mpack_start_array(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);

    if (count <= 15) {
        MPACK_WRITE_ENCODED(mpack_encode_fixarray, MPACK_TAG_SIZE_FIXARRAY, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_array16, MPACK_TAG_SIZE_ARRAY16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_array32, MPACK_TAG_SIZE_ARRAY32, (uint32_t)count);
    }

    mpack_writer_track_push(writer, mpack_type_array, count);
}

void mpack_start_map(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);

    if (count <= 15) {
        MPACK_WRITE_ENCODED(mpack_encode_fixmap, MPACK_TAG_SIZE_FIXMAP, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_map16, MPACK_TAG_SIZE_MAP16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_map32, MPACK_TAG_SIZE_MAP32, (uint32_t)count);
    }

    mpack_writer_track_push(writer, mpack_type_map, count);
}

static void mpack_start_str_notrack(mpack_writer_t* writer, uint32_t count) {
    if (count <= 31) {
        MPACK_WRITE_ENCODED(mpack_encode_fixstr, MPACK_TAG_SIZE_FIXSTR, (uint8_t)count);

    // str8 is only supported in v5 or later.
    } else if (count <= UINT8_MAX
            #if MPACK_COMPATIBILITY
            && writer->version >= mpack_version_v5
            #endif
            ) {
        MPACK_WRITE_ENCODED(mpack_encode_str8, MPACK_TAG_SIZE_STR8, (uint8_t)count);

    } else if (count <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_str16, MPACK_TAG_SIZE_STR16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_str32, MPACK_TAG_SIZE_STR32, (uint32_t)count);
    }
}

static void mpack_start_bin_notrack(mpack_writer_t* writer, uint32_t count) {
    #if MPACK_COMPATIBILITY
    // In the v4 spec, there was only the raw type for any kind of
    // variable-length data. In v4 mode, we support the bin functions,
    // but we produce an old-style raw.
    if (writer->version <= mpack_version_v4) {
        mpack_start_str_notrack(writer, count);
        return;
    }
    #endif

    if (count <= UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_bin8, MPACK_TAG_SIZE_BIN8, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_bin16, MPACK_TAG_SIZE_BIN16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_bin32, MPACK_TAG_SIZE_BIN32, (uint32_t)count);
    }
}

void mpack_start_str(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);
    mpack_start_str_notrack(writer, count);
    mpack_writer_track_push(writer, mpack_type_str, count);
}

void mpack_start_bin(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);
    mpack_start_bin_notrack(writer, count);
    mpack_writer_track_push(writer, mpack_type_bin, count);
}

void mpack_start_ext(mpack_writer_t* writer, int8_t exttype, uint32_t count) {
    #if MPACK_COMPATIBILITY
    if (writer->version <= mpack_version_v4) {
        mpack_break("Ext types require spec version v5 or later. This writer is in v%i mode.", (int)writer->version);
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }
    #endif

    mpack_writer_track_element(writer);

    if (count == 1) {
        MPACK_WRITE_ENCODED(mpack_encode_fixext1, MPACK_TAG_SIZE_FIXEXT1, exttype);
    } else if (count == 2) {
        MPACK_WRITE_ENCODED(mpack_encode_fixext2, MPACK_TAG_SIZE_FIXEXT2, exttype);
    } else if (count == 4) {
        MPACK_WRITE_ENCODED(mpack_encode_fixext4, MPACK_TAG_SIZE_FIXEXT4, exttype);
    } else if (count == 8) {
        MPACK_WRITE_ENCODED(mpack_encode_fixext8, MPACK_TAG_SIZE_FIXEXT8, exttype);
    } else if (count == 16) {
        MPACK_WRITE_ENCODED(mpack_encode_fixext16, MPACK_TAG_SIZE_FIXEXT16, exttype);
    } else if (count <= UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_ext8, MPACK_TAG_SIZE_EXT8, exttype, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_ext16, MPACK_TAG_SIZE_EXT16, exttype, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_ext32, MPACK_TAG_SIZE_EXT32, exttype, (uint32_t)count);
    }

    mpack_writer_track_push(writer, mpack_type_ext, count);
}



/*
 * Compound helpers and other functions
 */

void mpack_write_str(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_assert(data != NULL, "data for string of length %i is NULL", (int)count);
    mpack_start_str(writer, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_str(writer);
}

void mpack_write_bin(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_assert(data != NULL, "data pointer for bin of %i bytes is NULL", (int)count);
    mpack_start_bin(writer, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_bin(writer);
}

void mpack_write_ext(mpack_writer_t* writer, int8_t exttype, const char* data, uint32_t count) {
    mpack_assert(data != NULL, "data pointer for ext of type %i and %i bytes is NULL", exttype, (int)count);
    mpack_start_ext(writer, exttype, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_ext(writer);
}

void mpack_write_bytes(mpack_writer_t* writer, const char* data, size_t count) {
    mpack_assert(data != NULL, "data pointer for %i bytes is NULL", (int)count);
    mpack_writer_track_bytes(writer, count);
    mpack_write_native(writer, data, count);
}

void mpack_write_cstr(mpack_writer_t* writer, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr pointer is NULL");
    size_t length = mpack_strlen(cstr);
    if (length > UINT32_MAX)
        mpack_writer_flag_error(writer, mpack_error_invalid);
    mpack_write_str(writer, cstr, (uint32_t)length);
}

void mpack_write_cstr_or_nil(mpack_writer_t* writer, const char* cstr) {
    if (cstr)
        mpack_write_cstr(writer, cstr);
    else
        mpack_write_nil(writer);
}

void mpack_write_utf8(mpack_writer_t* writer, const char* str, uint32_t length) {
    mpack_assert(str != NULL, "data for string of length %i is NULL", (int)length);
    if (!mpack_utf8_check(str, length)) {
        mpack_writer_flag_error(writer, mpack_error_invalid);
        return;
    }
    mpack_write_str(writer, str, length);
}

void mpack_write_utf8_cstr(mpack_writer_t* writer, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr pointer is NULL");
    size_t length = mpack_strlen(cstr);
    if (length > UINT32_MAX)
        mpack_writer_flag_error(writer, mpack_error_invalid);
    mpack_write_utf8(writer, cstr, (uint32_t)length);
}

void mpack_write_utf8_cstr_or_nil(mpack_writer_t* writer, const char* cstr) {
    if (cstr)
        mpack_write_utf8_cstr(writer, cstr);
    else
        mpack_write_nil(writer);
}

#endif

