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

#include "mpack-writer.h"

#if MPACK_WRITER

#if MPACK_WRITE_TRACKING
#define MPACK_WRITER_TRACK(writer, error_expr) \
    (((writer)->error == mpack_ok) ? mpack_writer_flag_if_error((writer), (error_expr)) : ((void)0))

MPACK_STATIC_INLINE_SPEED void mpack_writer_flag_if_error(mpack_writer_t* writer, mpack_error_t error) {
    if (error != mpack_ok)
        mpack_writer_flag_error(writer, error);
}
#else
#define MPACK_WRITER_TRACK(writer, error_expr) MPACK_UNUSED(writer)
#endif

MPACK_STATIC_INLINE_SPEED void mpack_writer_track_element(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_element(&writer->track, false));
}

void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size) {
    mpack_assert(buffer != NULL, "cannot initialize writer with empty buffer");
    mpack_memset(writer, 0, sizeof(*writer));
    writer->buffer = buffer;
    writer->size = size;
    MPACK_WRITER_TRACK(writer, mpack_track_init(&writer->track));
}

void mpack_writer_init_error(mpack_writer_t* writer, mpack_error_t error) {
    mpack_memset(writer, 0, sizeof(*writer));
    writer->error = error;
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
    // We handle these here, making sure used is the total count in all three cases.
    mpack_log("flush size %i used %i data %p buffer %p\n", (int)writer->size, (int)writer->used, data, writer->buffer);

    // if the given data is not the old buffer, we'll need to actually copy it into the buffer
    bool is_extra_data = (data != writer->buffer);

    // if we're flushing all data (used is zero), we should actually grow
    size_t new_size = writer->size;
    if (writer->used == 0 && count != 0)
        new_size *= 2;
    while (new_size < (is_extra_data ? writer->used + count : count))
        new_size *= 2;

    if (new_size > writer->size) {
        mpack_log("flush growing from %i to %i\n", (int)writer->size, (int)new_size);

        char* new_buffer = (char*)mpack_realloc(writer->buffer, count, new_size);
        if (new_buffer == NULL) {
            mpack_writer_flag_error(writer, mpack_error_memory);
            return;
        }

        writer->buffer = new_buffer;
        writer->size = new_size;
    }

    if (is_extra_data) {
        mpack_memcpy(writer->buffer + writer->used, data, count);
        // add our extra data to count
        writer->used += count;
    } else {
        // used is either zero or count; set it to count
        writer->used = count;
    }
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

    MPACK_FREE(growable_writer);
    writer->context = NULL;
}

void mpack_writer_init_growable(mpack_writer_t* writer, char** target_data, size_t* target_size) {
    mpack_assert(target_data != NULL, "cannot initialize writer without a destination for the data");
    mpack_assert(target_size != NULL, "cannot initialize writer without a destination for the size");

    *target_data = NULL;
    *target_size = 0;

    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*) MPACK_MALLOC(sizeof(mpack_growable_writer_t));
    if (growable_writer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        return;
    }
    mpack_memset(growable_writer, 0, sizeof(*growable_writer));

    growable_writer->target_data = target_data;
    growable_writer->target_size = target_size;

    size_t capacity = MPACK_BUFFER_SIZE;
    char* buffer = (char*)MPACK_MALLOC(capacity);
    if (buffer == NULL) {
        MPACK_FREE(growable_writer);
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
typedef struct mpack_file_writer_t {
    FILE* file;
    char buffer[MPACK_BUFFER_SIZE];
} mpack_file_writer_t;

static void mpack_file_writer_flush(mpack_writer_t* writer, const char* buffer, size_t count) {
    mpack_file_writer_t* file_writer = (mpack_file_writer_t*)writer->context;
    size_t written = fwrite((const void*)buffer, 1, count, file_writer->file);
    if (written != count)
        mpack_writer_flag_error(writer, mpack_error_io);
}

static void mpack_file_writer_teardown(mpack_writer_t* writer) {
    mpack_file_writer_t* file_writer = (mpack_file_writer_t*)writer->context;

    if (file_writer->file) {
        int ret = fclose(file_writer->file);
        file_writer->file = NULL;
        if (ret != 0)
            mpack_writer_flag_error(writer, mpack_error_io);
    }

    MPACK_FREE(file_writer);
}

void mpack_writer_init_file(mpack_writer_t* writer, const char* filename) {
    mpack_assert(filename != NULL, "filename is NULL");

    mpack_file_writer_t* file_writer = (mpack_file_writer_t*) MPACK_MALLOC(sizeof(mpack_file_writer_t));
    if (file_writer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        return;
    }

    file_writer->file = fopen(filename, "wb");
    if (file_writer->file == NULL) {
        mpack_writer_init_error(writer, mpack_error_io);
        MPACK_FREE(file_writer);
        return;
    }

    mpack_writer_init(writer, file_writer->buffer, sizeof(file_writer->buffer));
    mpack_writer_set_context(writer, file_writer);
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

static void mpack_writer_flush_unchecked(mpack_writer_t* writer) {
    // This is a bit ugly; we reset used before calling flush so that
    // a flush function can distinguish between flushing the buffer
    // versus flushing external data. see mpack_growable_writer_flush()
    size_t used = writer->used;
    writer->used = 0;
    writer->flush(writer, writer->buffer, used);
}

static void mpack_write_native_big(mpack_writer_t* writer, const char* p, size_t count) {
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

    // we assume that the flush function is orders of magnitude slower
    // than memcpy(), so we fill the buffer up first to try to flush as
    // infrequently as possible.
    
    // fill the remaining space in the buffer
    size_t n = writer->size - writer->used;
    if (count < n)
        n = count;
    mpack_memcpy(writer->buffer + writer->used, p, n);
    writer->used += n;
    p += n;
    count -= n;
    if (count == 0)
        return;

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

// TODO obsolete remove, just always call big
MPACK_STATIC_INLINE_SPEED void mpack_write_native(mpack_writer_t* writer, const char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (writer->size - writer->used < count) {
        mpack_write_native_big(writer, p, count);
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
        case mpack_type_nil:    mpack_writer_track_element(writer); mpack_write_nil   (writer);            break;
        case mpack_type_bool:   mpack_writer_track_element(writer); mpack_write_bool  (writer, value.v.b); break;
        case mpack_type_float:  mpack_writer_track_element(writer); mpack_write_float (writer, value.v.f); break;
        case mpack_type_double: mpack_writer_track_element(writer); mpack_write_double(writer, value.v.d); break;
        case mpack_type_int:    mpack_writer_track_element(writer); mpack_write_int   (writer, value.v.i); break;
        case mpack_type_uint:   mpack_writer_track_element(writer); mpack_write_uint  (writer, value.v.u); break;

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

static size_t mpack_encode_u8(char* p, uint8_t value) {
    if (value <= 0x7f) {
        mpack_store_native_u8(p, (uint8_t)value);
        return 1;
    }
    mpack_store_native_u8(p, 0xcc);
    mpack_store_native_u8(p + 1, (uint8_t)value);
    return 2;
}

static size_t mpack_encode_u16(char* p, uint16_t value) {
    if (value <= 0x7f) {
        mpack_store_native_u8(p, (uint8_t)value);
        return 1;
    }
    if (value <= UINT8_MAX) {
        mpack_store_native_u8(p, 0xcc);
        mpack_store_native_u8(p + 1, (uint8_t)value);
        return 2;
    }
    mpack_store_native_u8(p, 0xcd);
    mpack_store_native_u16(p + 1, value);
    return 3;
}

static size_t mpack_encode_u32(char* p, uint32_t value) {
    if (value <= 0x7f) {
        mpack_store_native_u8(p, (uint8_t)value);
        return 1;
    }
    if (value <= UINT8_MAX) {
        mpack_store_native_u8(p, 0xcc);
        mpack_store_native_u8(p + 1, (uint8_t)value);
        return 2;
    }
    if (value <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xcd);
        mpack_store_native_u16(p + 1, (uint16_t)value);
        return 3;
    }
    mpack_store_native_u8(p, 0xce);
    mpack_store_native_u32(p + 1, value);
    return 5;
}

static size_t mpack_encode_u64(char* p, uint64_t value) {
    if (value <= 0x7f) {
        mpack_store_native_u8(p, (uint8_t)value);
        return 1;
    }
    if (value <= UINT8_MAX) {
        mpack_store_native_u8(p, 0xcc);
        mpack_store_native_u8(p + 1, (uint8_t)value);
        return 2;
    }
    if (value <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xcd);
        mpack_store_native_u16(p + 1, (uint16_t)value);
        return 3;
    }
    if (value <= UINT32_MAX) {
        mpack_store_native_u8(p, 0xce);
        mpack_store_native_u32(p + 1, (uint32_t)value);
        return 5;
    }
    mpack_store_native_u8(p, 0xcf);
    mpack_store_native_u64(p + 1, value);
    return 9;
}

static size_t mpack_encode_i8(char* p, int8_t value) {

    // write any non-negative number as a uint
    if (value >= 0)
        return mpack_encode_u8(p, (uint8_t)value);

    if (value >= -32) {
        mpack_store_native_i8(p, value);
        return 1;
    }
    mpack_store_native_u8(p, 0xd0);
    mpack_store_native_i8(p + 1, value);
    return 2;
}

static size_t mpack_encode_i16(char* p, int16_t value) {

    // write any non-negative number as a uint
    if (value >= 0)
        return mpack_encode_u16(p, (uint16_t)value);

    if (value >= -32) {
        mpack_store_native_i8(p, (int8_t)value);
        return 1;
    }
    if (value >= INT8_MIN) {
        mpack_store_native_u8(p, 0xd0);
        mpack_store_native_i8(p + 1, (int8_t)value);
        return 2;
    }
    mpack_store_native_u8(p, 0xd1);
    mpack_store_native_i16(p + 1, value);
    return 3;
}

static size_t mpack_encode_i32(char* p, int32_t value) {

    // write any non-negative number as a uint
    if (value >= 0)
        return mpack_encode_u32(p, (uint32_t)value);

    if (value >= -32) {
        mpack_store_native_i8(p, (int8_t)value);
        return 1;
    }
    if (value >= INT8_MIN) {
        mpack_store_native_u8(p, 0xd0);
        mpack_store_native_i8(p + 1, (int8_t)value);
        return 2;
    }
    if (value >= INT16_MIN) {
        mpack_store_native_u8(p, 0xd1);
        mpack_store_native_i16(p + 1, (int16_t)value);
        return 3;
    }
    mpack_store_native_u8(p, 0xd2);
    mpack_store_native_i32(p + 1, value);
    return 5;

}

static size_t mpack_encode_i64(char* p, int64_t value) {

    // write any non-negative number as a uint
    if (value >= 0)
        return mpack_encode_u64(p, (uint64_t)value);

    if (value >= -32) {
        mpack_store_native_i8(p, (int8_t)value);
        return 1;
    }
    if (value >= INT8_MIN) {
        mpack_store_native_u8(p, 0xd0);
        mpack_store_native_i8(p + 1, (int8_t)value);
        return 2;
    }
    if (value >= INT16_MIN) {
        mpack_store_native_u8(p, 0xd1);
        mpack_store_native_i16(p + 1, (int16_t)value);
        return 3;
    }
    if (value >= INT32_MIN) {
        mpack_store_native_u8(p, 0xd2);
        mpack_store_native_i32(p + 1, (int32_t)value);
        return 5;
    }
    mpack_store_native_u8(p, 0xd3);
    mpack_store_native_i64(p + 1, value);
    return 9;
}

MPACK_STATIC_ALWAYS_INLINE void mpack_write_byte_element(mpack_writer_t* writer, char value) {
    mpack_writer_track_element(writer);
    mpack_write_native(writer, &value, 1);
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

void mpack_write_nil(mpack_writer_t* writer) {
    mpack_write_byte_element(writer, (char)0xc0);
}

MPACK_STATIC_INLINE size_t mpack_encode_float(char* p, float value) {
    mpack_store_native_u8(p, 0xca);
    mpack_store_native_float(p + 1, value);
    return 5;
}

MPACK_STATIC_INLINE size_t mpack_encode_double(char* p, double value) {
    mpack_store_native_u8(p, 0xcb);
    mpack_store_native_double(p + 1, value);
    return 9;
}

#if MPACK_WRITE_TRACKING
void mpack_finish_array(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, mpack_type_array));
}

void mpack_finish_map(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, mpack_type_map));
}

void mpack_finish_str(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, mpack_type_str));
}

void mpack_finish_bin(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, mpack_type_bin));
}

void mpack_finish_ext(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, mpack_type_ext));
}

void mpack_finish_type(mpack_writer_t* writer, mpack_type_t type) {
    MPACK_WRITER_TRACK(writer, mpack_track_pop(&writer->track, type));
}
#endif

static size_t mpack_encode_array(char* p, uint32_t count) {
    if (count <= 15) {
        mpack_store_native_u8(p, (uint8_t)(0x90 | count));
        return 1;
    }
    if (count <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xdc);
        mpack_store_native_u16(p + 1, (uint16_t)count);
        return 3;
    }
    mpack_store_native_u8(p, 0xdd);
    mpack_store_native_u32(p + 1, count);
    return 5;
}

static size_t mpack_encode_map(char* p, uint32_t count) {
    if (count <= 15) {
        mpack_store_native_u8(p, (uint8_t)(0x80 | count));
        return 1;
    }
    if (count <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xde);
        mpack_store_native_u16(p + 1, (uint16_t)count);
        return 3;
    }
    mpack_store_native_u8(p, 0xdf);
    mpack_store_native_u32(p + 1, count);
    return 5;
}

static size_t mpack_encode_str(char* p, uint32_t count) {
    if (count <= 31) {
        mpack_store_native_u8(p, (uint8_t)(0xa0 | count));
        return 1;
    }
    if (count <= UINT8_MAX) {
        // TODO: str8 had no counterpart in MessagePack 1.0; there was only
        // raw16 and raw32. This should not be used in compatibility mode.
        mpack_store_native_u8(p, 0xd9);
        mpack_store_native_u8(p + 1, (uint8_t)count);
        return 2;
    }
    if (count <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xda);
        mpack_store_native_u16(p + 1, (uint16_t)count);
        return 3;
    }
    mpack_store_native_u8(p, 0xdb);
    mpack_store_native_u32(p + 1, count);
    return 5;
}

static size_t mpack_encode_bin(char* p, uint32_t count) {
    if (count <= UINT8_MAX) {
        mpack_store_native_u8(p, 0xc4);
        mpack_store_native_u8(p + 1, (uint8_t)count);
        return 2;
    }
    if (count <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xc5);
        mpack_store_native_u16(p + 1, (uint16_t)count);
        return 3;
    }
    mpack_store_native_u8(p, 0xc6);
    mpack_store_native_u32(p + 1, count);
    return 5;
}

static size_t mpack_encode_ext(char* p, int8_t exttype, uint32_t count) {
    if (count == 1) {
        mpack_store_native_u8(p, 0xd4);
        mpack_store_native_i8(p + 1, exttype);
        return 2;
    }
    if (count == 2) {
        mpack_store_native_u8(p, 0xd5);
        mpack_store_native_i8(p + 1, exttype);
        return 2;
    }
    if (count == 4) {
        mpack_store_native_u8(p, 0xd6);
        mpack_store_native_i8(p + 1, exttype);
        return 2;
    }
    if (count == 8) {
        mpack_store_native_u8(p, 0xd7);
        mpack_store_native_i8(p + 1, exttype);
        return 2;
    }
    if (count == 16) {
        mpack_store_native_u8(p, 0xd8);
        mpack_store_native_i8(p + 1, exttype);
        return 2;
    }
    if (count <= UINT8_MAX) {
        mpack_store_native_u8(p, 0xc7);
        mpack_store_native_u8(p + 1, (uint8_t)count);
        mpack_store_native_i8(p + 2, exttype);
        return 3;
    }
    if (count <= UINT16_MAX) {
        mpack_store_native_u8(p, 0xc8);
        mpack_store_native_u16(p + 1, (uint16_t)count);
        mpack_store_native_i8(p + 3, exttype);
        return 4;
    }
    mpack_store_native_u8(p, 0xc9);
    mpack_store_native_u32(p + 1, count);
    mpack_store_native_i8(p + 5, exttype);
    return 6;
}

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
    MPACK_WRITER_TRACK(writer, mpack_track_bytes(&writer->track, false, count));
    if (mpack_writer_error(writer) != mpack_ok)
        return;
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

// We use a macro to define the content of type wrappers since they are
// almost all identical. The function definitions are still spelled
// out for intellisense/etc.

#define MPACK_WRITE_WRAPPER(name, maximum_possible_bytes, ...)                          \
    mpack_writer_track_element(writer);                                                 \
    if (mpack_writer_buffer_left(writer) >= maximum_possible_bytes) {                   \
        writer->used += mpack_encode_##name(writer->buffer + writer->used, __VA_ARGS__); \
    } else if (mpack_writer_error(writer) == mpack_ok) {                                \
        if (writer->flush)                                                              \
            mpack_writer_flush_unchecked(writer);                                       \
        char buf[maximum_possible_bytes];                                               \
        mpack_write_native(writer, buf, mpack_encode_##name(buf, __VA_ARGS__));          \
    }

void mpack_write_u8 (mpack_writer_t* writer, uint8_t value)  {MPACK_WRITE_WRAPPER(u8,  2, value);}
void mpack_write_u16(mpack_writer_t* writer, uint16_t value) {MPACK_WRITE_WRAPPER(u16, 3, value);}
void mpack_write_u32(mpack_writer_t* writer, uint32_t value) {MPACK_WRITE_WRAPPER(u32, 5, value);}
void mpack_write_u64(mpack_writer_t* writer, uint64_t value) {MPACK_WRITE_WRAPPER(u64, 9, value);}

void mpack_write_i8 (mpack_writer_t* writer, int8_t value)   {MPACK_WRITE_WRAPPER(i8,  2, value);}
void mpack_write_i16(mpack_writer_t* writer, int16_t value)  {MPACK_WRITE_WRAPPER(i16, 3, value);}
void mpack_write_i32(mpack_writer_t* writer, int32_t value)  {MPACK_WRITE_WRAPPER(i32, 5, value);}
void mpack_write_i64(mpack_writer_t* writer, int64_t value)  {MPACK_WRITE_WRAPPER(i64, 9, value);}

void mpack_write_float(mpack_writer_t* writer, float value)   {MPACK_WRITE_WRAPPER(float, 5, value);}
void mpack_write_double(mpack_writer_t* writer, double value) {MPACK_WRITE_WRAPPER(double, 9, value);}

void mpack_start_array(mpack_writer_t* writer, uint32_t count) {
    MPACK_WRITE_WRAPPER(array, 5, count);
    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_array, count));
}

void mpack_start_map(mpack_writer_t* writer, uint32_t count) {
    MPACK_WRITE_WRAPPER(map, 5, count);
    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_map, count));
}

void mpack_start_str(mpack_writer_t* writer, uint32_t count) {
    MPACK_WRITE_WRAPPER(str, 5, count);
    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_str, count));
}

void mpack_start_bin(mpack_writer_t* writer, uint32_t count) {
    MPACK_WRITE_WRAPPER(bin, 5, count);
    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_bin, count));
}

void mpack_start_ext(mpack_writer_t* writer, int8_t exttype, uint32_t count) {
    MPACK_WRITE_WRAPPER(ext, 6, exttype, count);
    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_ext, count));
}

#endif

