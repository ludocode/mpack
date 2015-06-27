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
#define MPACK_WRITER_TRACK(writer, error) mpack_writer_flag_if_error(writer, error)

static inline void mpack_writer_flag_if_error(mpack_writer_t* writer, mpack_error_t error) {
    if (error != mpack_ok)
        mpack_writer_flag_error(writer, error);
}
#else
#define MPACK_WRITER_TRACK(writer, error) MPACK_UNUSED(writer)
#endif

static inline void mpack_writer_track_element(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_element(&writer->track, true));
}

void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size) {
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
    mpack_writer_t* writer; // this causes the writer to not be movable...

    char* data;
    size_t size;
    size_t capacity;

    char** target_data;
    size_t* target_size;

    char buffer[MPACK_BUFFER_SIZE];
} mpack_growable_writer_t;

static bool mpack_growable_writer_flush(void* context, const char* buffer, size_t count) {
    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*)context;

    if (growable_writer->size + count > growable_writer->capacity) {
        size_t new_capacity = growable_writer->size + count;
        if (new_capacity < growable_writer->capacity * 2)
            new_capacity = growable_writer->capacity * 2;

        char* new_data = (char*)MPACK_MALLOC(new_capacity);
        if (new_data == NULL) {
            mpack_writer_flag_error(growable_writer->writer, mpack_error_memory);
            return false;
        }

        if (growable_writer->data) {
            mpack_memcpy(new_data, growable_writer->data, growable_writer->size);
            MPACK_FREE(growable_writer->data);
        }
        growable_writer->data = new_data;
        growable_writer->capacity = new_capacity;
    }

    mpack_memcpy(growable_writer->data + growable_writer->size, buffer, count);
    growable_writer->size += count;
    return true;
}

static void mpack_growable_writer_teardown(void* context) {
    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*)context;

    if (mpack_writer_error(growable_writer->writer) == mpack_ok) {
        *growable_writer->target_data = growable_writer->data;
        *growable_writer->target_size = growable_writer->size;

    } else if (growable_writer->data) {
        MPACK_FREE(growable_writer->data);
    }

    MPACK_FREE(growable_writer);
}

void mpack_writer_init_growable(mpack_writer_t* writer, char** data, size_t* size) {
    *data = NULL;
    *size = 0;

    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*) MPACK_MALLOC(sizeof(mpack_growable_writer_t));
    if (growable_writer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        return;
    }
    mpack_memset(growable_writer, 0, sizeof(*growable_writer));

    growable_writer->writer = writer;
    growable_writer->target_data = data;
    growable_writer->target_size = size;

    mpack_writer_init(writer, growable_writer->buffer, sizeof(growable_writer->buffer));
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

static bool mpack_file_writer_flush(void* context, const char* buffer, size_t count) {
    mpack_file_writer_t* file_writer = (mpack_file_writer_t*)context;
    size_t written = fwrite((const void*)buffer, 1, count, file_writer->file);
    return written == count;
}

static void mpack_file_writer_teardown(void* context) {
    mpack_file_writer_t* file_writer = (mpack_file_writer_t*)context;
    if (file_writer->file)
        fclose(file_writer->file);
    MPACK_FREE(file_writer);
}

void mpack_writer_init_file(mpack_writer_t* writer, const char* filename) {
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

    if (!writer->error) {
        writer->error = error;
        #if MPACK_SETJMP
        if (writer->jump)
            longjmp(writer->jump_env, 1);
        #endif
    }
}

static void mpack_write_native_big(mpack_writer_t* writer, const char* p, size_t count) {
    if (writer->error)
        return;
    mpack_log("big write for %i bytes from %p, %i space left in buffer\n",
            (int)count, p, (int)(writer->size - writer->used));
    mpack_assert(count > writer->size - writer->used,
            "big write requested for %i bytes, but there is %i available "
            "space in buffer. call mpack_write_native() instead",
            (int)count, (int)(writer->size - writer->used));

    // we assume that the flush function is orders of magnitude slower
    // than memcpy(), so we always fill the buffer as much as possible
    // and write only in multiples of the buffer size.
    
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
    if (!writer->flush || !writer->flush(writer->context, writer->buffer, writer->used)) {
        mpack_writer_flag_error(writer, mpack_error_io);
        return;
    }
    writer->used = 0;

    // flush any data that doesn't fit in the buffer
    n = count - (count % writer->size);
    if (n > 0) {
        if (!writer->flush(writer->context, p, n)) {
            mpack_writer_flag_error(writer, mpack_error_io);
            return;
        }
        p += n;
        count -= n;
    }

    // copy any remaining data into the buffer
    mpack_memcpy(writer->buffer, p, count);
    writer->used += count;
}

static inline void mpack_write_native(mpack_writer_t* writer, const char* p, size_t count) {
    if (writer->error != mpack_ok)
        return;

    #if 0
    // useful for debugging unit tests
    mpack_log("writing:");
    for (size_t i = 0; i < count; ++i)
        mpack_log(" %02x", (uint8_t)p[i]);
    mpack_log("\n");
    #endif

    if (writer->size - writer->used < count) {
        mpack_write_native_big(writer, p, count);
    } else {
        mpack_memcpy(writer->buffer + writer->used, p, count);
        writer->used += count;
    }
}

static void mpack_write_native_u8(mpack_writer_t* writer, uint8_t val) {
    char c[] = {
        (char)val
    };
    mpack_write_native(writer, c, sizeof(c));
}

static void mpack_write_native_u16(mpack_writer_t* writer, uint16_t val) {
    char c[] = {
        (char)(uint8_t)((val >> 8) & 0xFF),
        (char)(uint8_t)( val       & 0xFF)
    };
    mpack_write_native(writer, c, sizeof(c));
}

static void mpack_write_native_u32(mpack_writer_t* writer, uint32_t val) {
    char c[] = {
        (char)(uint8_t)((val >> 24) & 0xFF),
        (char)(uint8_t)((val >> 16) & 0xFF),
        (char)(uint8_t)((val >>  8) & 0xFF),
        (char)(uint8_t)( val        & 0xFF)
    };
    mpack_write_native(writer, c, sizeof(c));
}

static void mpack_write_native_u64(mpack_writer_t* writer, uint64_t val) {
    char c[] = {
        (char)(uint8_t)((val >> 56) & 0xFF),
        (char)(uint8_t)((val >> 48) & 0xFF),
        (char)(uint8_t)((val >> 40) & 0xFF),
        (char)(uint8_t)((val >> 32) & 0xFF),
        (char)(uint8_t)((val >> 24) & 0xFF),
        (char)(uint8_t)((val >> 16) & 0xFF),
        (char)(uint8_t)((val >>  8) & 0xFF),
        (char)(uint8_t)( val        & 0xFF)
    };
    mpack_write_native(writer, c, sizeof(c));
}

static inline void mpack_write_native_i8  (mpack_writer_t* writer,  int8_t  val) {mpack_write_native_u8  (writer, (uint8_t )val);}
static inline void mpack_write_native_i16 (mpack_writer_t* writer,  int16_t val) {mpack_write_native_u16 (writer, (uint16_t)val);}
static inline void mpack_write_native_i32 (mpack_writer_t* writer,  int32_t val) {mpack_write_native_u32 (writer, (uint32_t)val);}
static inline void mpack_write_native_i64 (mpack_writer_t* writer,  int64_t val) {mpack_write_native_u64 (writer, (uint64_t)val);}


static void mpack_write_native_float(mpack_writer_t* writer, float value) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = value;
    mpack_write_native_u32(writer, u.i);
}

static void mpack_write_native_double(mpack_writer_t* writer, double value) {
    union {
        double d;
        uint64_t i;
    } u;
    u.d = value;
    mpack_write_native_u64(writer, u.i);
}

mpack_error_t mpack_writer_destroy(mpack_writer_t* writer) {
    MPACK_WRITER_TRACK(writer, mpack_track_destroy(&writer->track, false));

    // flush any outstanding data
    if (writer->error == mpack_ok && writer->used != 0 && writer->flush != NULL) {
        if (!writer->flush(writer->context, writer->buffer, writer->used)) {
            mpack_writer_flag_error(writer, mpack_error_io);
        }
        writer->used = 0;
    }

    if (writer->teardown)
        writer->teardown(writer->context);
    writer->teardown = NULL;

    return writer->error;
}

void mpack_write_tag(mpack_writer_t* writer, mpack_tag_t value) {
    mpack_writer_track_element(writer);

    switch (value.type) {

        case mpack_type_nil:    mpack_write_nil   (writer);          break;

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

void mpack_write_u8(mpack_writer_t* writer, uint8_t value) {
    mpack_writer_track_element(writer);
    if (value <= 0x7f) {
        mpack_write_native_u8(writer, (uint8_t)value);
    } else {
        mpack_write_native_u8(writer, 0xcc);
        mpack_write_native_u8(writer, (uint8_t)value);
    }
}

void mpack_write_u16(mpack_writer_t* writer, uint16_t value) {
    mpack_writer_track_element(writer);
    if (value <= 0x7f) {
        mpack_write_native_u8(writer, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        mpack_write_native_u8(writer, 0xcc);
        mpack_write_native_u8(writer, (uint8_t)value);
    } else {
        mpack_write_native_u8(writer, 0xcd);
        mpack_write_native_u16(writer, value);
    }
}

void mpack_write_u32(mpack_writer_t* writer, uint32_t value) {
    mpack_writer_track_element(writer);
    if (value <= 0x7f) {
        mpack_write_native_u8(writer, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        mpack_write_native_u8(writer, 0xcc);
        mpack_write_native_u8(writer, (uint8_t)value);
    } else if (value <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xcd);
        mpack_write_native_u16(writer, (uint16_t)value);
    } else {
        mpack_write_native_u8(writer, 0xce);
        mpack_write_native_u32(writer, value);
    }
}

void mpack_write_u64(mpack_writer_t* writer, uint64_t value) {
    mpack_writer_track_element(writer);
    if (value <= 0x7f) {
        mpack_write_native_u8(writer, (uint8_t)value);
    } else if (value <= UINT8_MAX) {
        mpack_write_native_u8(writer, 0xcc);
        mpack_write_native_u8(writer, (uint8_t)value);
    } else if (value <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xcd);
        mpack_write_native_u16(writer, (uint16_t)value);
    } else if (value <= UINT32_MAX) {
        mpack_write_native_u8(writer, 0xce);
        mpack_write_native_u32(writer, (uint32_t)value);
    } else {
        mpack_write_native_u8(writer, 0xcf);
        mpack_write_native_u64(writer, value);
    }
}

void mpack_write_i8(mpack_writer_t* writer, int8_t value) {

    // write any non-negative number as a uint
    if (value >= 0) {
        mpack_write_u8(writer, (uint8_t)value);
        return;
    }

    mpack_writer_track_element(writer);
    if (value >= -32) {
        mpack_write_native_i8(writer, (int8_t)0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
    } else {
        mpack_write_native_u8(writer, 0xd0);
        mpack_write_native_i8(writer, value);
    }

}

void mpack_write_i16(mpack_writer_t* writer, int16_t value) {

    // write any non-negative number as a uint
    if (value >= 0) {
        mpack_write_u16(writer, (uint16_t)value);
        return;
    }

    mpack_writer_track_element(writer);
    if (value >= -32) {
        mpack_write_native_i8(writer, (int8_t)0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
    } else if (value >= INT8_MIN) {
        mpack_write_native_u8(writer, 0xd0);
        mpack_write_native_i8(writer, (int8_t)value);
    } else {
        mpack_write_native_u8(writer, 0xd1);
        mpack_write_native_i16(writer, value);
    }

}

void mpack_write_i32(mpack_writer_t* writer, int32_t value) {

    // write any non-negative number as a uint
    if (value >= 0) {
        mpack_write_u32(writer, (uint32_t)value);
        return;
    }

    mpack_writer_track_element(writer);
    if (value >= -32) {
        mpack_write_native_i8(writer, (int8_t)0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
    } else if (value >= INT8_MIN) {
        mpack_write_native_u8(writer, 0xd0);
        mpack_write_native_i8(writer, (int8_t)value);
    } else if (value >= INT16_MIN) {
        mpack_write_native_u8(writer, 0xd1);
        mpack_write_native_i16(writer, (int16_t)value);
    } else {
        mpack_write_native_u8(writer, 0xd2);
        mpack_write_native_i32(writer, value);
    }

}

void mpack_write_i64(mpack_writer_t* writer, int64_t value) {

    // write any non-negative number as a uint
    if (value >= 0) {
        mpack_write_u64(writer, (uint64_t)value);
        return;
    }

    mpack_writer_track_element(writer);
    if (value >= -32) {
        mpack_write_native_i8(writer, (int8_t)0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
    } else if (value >= INT8_MIN) {
        mpack_write_native_u8(writer, 0xd0);
        mpack_write_native_i8(writer, (int8_t)value);
    } else if (value >= INT16_MIN) {
        mpack_write_native_u8(writer, 0xd1);
        mpack_write_native_i16(writer, (int16_t)value);
    } else if (value >= INT32_MIN) {
        mpack_write_native_u8(writer, 0xd2);
        mpack_write_native_i32(writer, (int32_t)value);
    } else {
        mpack_write_native_u8(writer, 0xd3);
        mpack_write_native_i64(writer, value);
    }

}

void mpack_write_bool(mpack_writer_t* writer, bool value) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, (uint8_t)(0xc2 | (value ? 1 : 0)));
}

void mpack_write_true(mpack_writer_t* writer) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, (uint8_t)0xc3);
}

void mpack_write_false(mpack_writer_t* writer) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, (uint8_t)0xc2);
}

void mpack_write_nil(mpack_writer_t* writer) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, 0xc0);
}

void mpack_write_float(mpack_writer_t* writer, float value) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, 0xca);
    mpack_write_native_float(writer, value);
}

void mpack_write_double(mpack_writer_t* writer, double value) {
    mpack_writer_track_element(writer);
    mpack_write_native_u8(writer, 0xcb);
    mpack_write_native_double(writer, value);
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
#endif

void mpack_start_array(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_writer_track_element(writer);
    if (count <= 15) {
        mpack_write_native_u8(writer, (uint8_t)(0x90 | count));
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xdc);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xdd);
        mpack_write_native_u32(writer, count);
    }

    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_array, count));
}

void mpack_start_map(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_writer_track_element(writer);
    if (count <= 15) {
        mpack_write_native_u8(writer, (uint8_t)(0x80 | count));
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xde);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xdf);
        mpack_write_native_u32(writer, count);
    }

    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_map, count));
}

void mpack_start_str(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_writer_track_element(writer);
    if (count <= 31) {
        mpack_write_native_u8(writer, (uint8_t)(0xa0 | count));
    } else if (count <= UINT8_MAX) {
        // TODO: THIS NOT AVAILABLE IN COMPATIBILITY MODE?? was not in 1.0?
        mpack_write_native_u8(writer, 0xd9);
        mpack_write_native_u8(writer, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xda);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xdb);
        mpack_write_native_u32(writer, count);
    }

    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_str, count));
}

void mpack_start_bin(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_writer_track_element(writer);
    if (count <= UINT8_MAX) {
        mpack_write_native_u8(writer, 0xc4);
        mpack_write_native_u8(writer, (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xc5);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xc6);
        mpack_write_native_u32(writer, count);
    }

    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_bin, count));
}

void mpack_start_ext(mpack_writer_t* writer, int8_t exttype, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    // TODO: fail if compatibility mode

    mpack_writer_track_element(writer);
    if (count == 1) {
        mpack_write_native_u8(writer, 0xd4);
        mpack_write_native_i8(writer, exttype);
    } else if (count == 2) {
        mpack_write_native_u8(writer, 0xd5);
        mpack_write_native_i8(writer, exttype);
    } else if (count == 4) {
        mpack_write_native_u8(writer, 0xd6);
        mpack_write_native_i8(writer, exttype);
    } else if (count == 8) {
        mpack_write_native_u8(writer, 0xd7);
        mpack_write_native_i8(writer, exttype);
    } else if (count == 16) {
        mpack_write_native_u8(writer, 0xd8);
        mpack_write_native_i8(writer, exttype);
    } else if (count <= UINT8_MAX) {
        mpack_write_native_u8(writer, 0xc7);
        mpack_write_native_u8(writer, (uint8_t)count);
        mpack_write_native_i8(writer, exttype);
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xc8);
        mpack_write_native_u16(writer, (uint16_t)count);
        mpack_write_native_i8(writer, exttype);
    } else {
        mpack_write_native_u8(writer, 0xc9);
        mpack_write_native_u32(writer, count);
        mpack_write_native_i8(writer, exttype);
    }

    MPACK_WRITER_TRACK(writer, mpack_track_push(&writer->track, mpack_type_ext, count));
}

void mpack_write_str(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_start_str(writer, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_str(writer);
}

void mpack_write_bin(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_start_bin(writer, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_bin(writer);
}

void mpack_write_ext(mpack_writer_t* writer, int8_t exttype, const char* data, uint32_t count) {
    mpack_start_ext(writer, exttype, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_ext(writer);
}

void mpack_write_bytes(mpack_writer_t* writer, const char* data, size_t count) {
    MPACK_WRITER_TRACK(writer, mpack_track_bytes(&writer->track, false, count));
    mpack_write_native(writer, data, count);
}

void mpack_write_cstr(mpack_writer_t* writer, const char* str) {
    size_t len = mpack_strlen(str);
    if (len > UINT32_MAX)
        mpack_writer_flag_error(writer, mpack_error_invalid);
    mpack_write_str(writer, str, (uint32_t)len);
}

#endif

