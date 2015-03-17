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

#include "mpack-writer.h"

#if MPACK_WRITER

#if MPACK_TRACKING
struct mpack_writer_track_t {
    struct mpack_writer_track_t* next;
    mpack_type_t type;
    uint64_t count;
};
#endif

static void mpack_writer_add_track(mpack_writer_t* writer, mpack_type_t type, uint64_t count) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(type);
    MPACK_UNUSED(count);
    #if MPACK_TRACKING
    mpack_writer_track_t* track = (mpack_writer_track_t*) MPACK_MALLOC(sizeof(mpack_writer_track_t));
    if (!track) {
        mpack_writer_flag_error(writer, mpack_error_memory);
        return;
    }
    track->next = writer->track;
    track->type = type;
    track->count = count;
    writer->track = track;
    #endif
}

#if MPACK_TRACKING
static void mpack_finish_track(mpack_writer_t* writer, mpack_type_t type) {
    if (writer->error != mpack_ok)
        return;

    if (!writer->track) {
        mpack_assert(0, "attempting to close a %s but nothing was opened!", mpack_type_to_string(type));
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    if (writer->track->type != type) {
        mpack_assert(0, "attempting to close a %s but the open element is a %s!",
                mpack_type_to_string(type), mpack_type_to_string(writer->track->type));
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    if (writer->track->count != 0) {
        mpack_assert(0, "attempting to close a %s but there are %"PRIu64" left",
                mpack_type_to_string(type), writer->track->count);
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    mpack_writer_track_t* track = writer->track;
    writer->track = track->next;
    MPACK_FREE(track);
}

static void mpack_track_write(mpack_writer_t* writer, bool bytes, uint64_t count) {
    if (writer->error != mpack_ok)
        return;

    if (writer->track) {

        // make sure it's the right type
        if (bytes) {
            if (writer->track->type == mpack_type_map || writer->track->type == mpack_type_array) {
                mpack_assert(0, "bytes cannot be written within an %s", mpack_type_to_string(writer->track->type));
                mpack_writer_flag_error(writer, mpack_error_bug);
            }
        } else {
            if (writer->track->type != mpack_type_map && writer->track->type != mpack_type_array) {
                mpack_assert(0, "elements cannot be written within an %s", mpack_type_to_string(writer->track->type));
                mpack_writer_flag_error(writer, mpack_error_bug);
            }
        }

        // make sure we don't overflow
        if (writer->track->count < count) {
            mpack_assert(0, "too many elements/bytes written for %s", mpack_type_to_string(writer->track->type));
            mpack_writer_flag_error(writer, mpack_error_bug);
        }
        writer->track->count -= count;

    }
}
#endif

static inline void mpack_track_element_write(mpack_writer_t* writer) {
    MPACK_UNUSED(writer);
    #if MPACK_TRACKING
    mpack_track_write(writer, false, 1);
    #endif
}

static inline void mpack_track_bytes_written(mpack_writer_t* writer, uint64_t count) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(count);
    #if MPACK_TRACKING
    mpack_track_write(writer, true, count);
    #endif
}

void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size) {
    memset(writer, 0, sizeof(*writer));
    writer->buffer = buffer;
    writer->size = size;
}

void mpack_writer_flag_error(mpack_writer_t* writer, mpack_error_t error) {
    mpack_log("writer %p setting error %i: %s\n", writer, (int)error, mpack_error_to_string(error));

    #if MPACK_TRACKING
    while (writer->track) {
        mpack_writer_track_t* track = writer->track;
        writer->track = track->next;
        MPACK_FREE(track);
    }
    #endif

    if (!writer->error) {
        writer->error = error;
        if (writer->jump)
            longjmp(writer->jump_env, 1);
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
    memcpy(writer->buffer + writer->used, p, n);
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
    memcpy(writer->buffer, p, count);
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
        memcpy(writer->buffer + writer->used, p, count);
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
static inline void mpack_write_native_bool(mpack_writer_t* writer,  bool    val) {mpack_write_native_u8  (writer, (uint8_t )val);}


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
    #if MPACK_TRACKING
    if (writer->error == mpack_ok && writer->track) {
        mpack_assert(0, "writer has an unclosed %s", mpack_type_to_string(writer->track->type));
        mpack_writer_flag_error(writer, mpack_error_bug);
    }
    #endif

    // flush any outstanding data
    if (writer->error == mpack_ok && writer->used != 0 && writer->flush != NULL) {
        if (!writer->flush(writer->context, writer->buffer, writer->used)) {
            mpack_writer_flag_error(writer, mpack_error_io);
        }
        writer->used = 0;
    }

    if (writer->teardown)
        writer->teardown(writer->context);

    return writer->error;
}

void mpack_write_tag(mpack_writer_t* writer, mpack_tag_t value) {
    mpack_track_element_write(writer);

    switch (value.type) {

        case mpack_type_nil:    mpack_write_nil   (writer);          break;
        case mpack_type_bool:   mpack_write_bool  (writer, value.v.b); break;
        case mpack_type_float:  mpack_write_float (writer, value.v.f); break;
        case mpack_type_double: mpack_write_double(writer, value.v.d); break;
        case mpack_type_int:    mpack_write_int   (writer, value.v.i); break;
        case mpack_type_uint:   mpack_write_uint  (writer, value.v.u); break;

        case mpack_type_str:
            if (value.v.u > UINT32_MAX) {
                mpack_assert(0, "str has too many bytes for a 32-bit uint: %"PRIu64, value.v.u);
                mpack_writer_flag_error(writer, mpack_error_bug);
            }
            mpack_start_str(writer, (uint32_t)value.v.u);
            break;

        case mpack_type_array:
            if (value.v.u > UINT32_MAX) {
                mpack_assert(0, "array has too many elements for a 32-bit int: %"PRIu64, value.v.u);
                mpack_writer_flag_error(writer, mpack_error_bug);
            }
            mpack_start_array(writer, (uint32_t)value.v.u);
            break;

        case mpack_type_map:
            if (value.v.u > UINT32_MAX) {
                mpack_assert(0, "map has too many elements for a 32-bit int: %"PRIu64, value.v.u);
                mpack_writer_flag_error(writer, mpack_error_bug);
            }
            mpack_start_map(writer, (uint32_t)value.v.u);
            break;

        default:
            mpack_assert(0, "unrecognized type %i", (int)value.type);
            mpack_writer_flag_error(writer, mpack_error_bug);
            break;
    }
}

void mpack_write_u8(mpack_writer_t* writer, uint8_t value) {
    mpack_track_element_write(writer);
    if (value <= 0x7f) {
        mpack_write_native_u8(writer, (uint8_t)value);
    } else {
        mpack_write_native_u8(writer, 0xcc);
        mpack_write_native_u8(writer, (uint8_t)value);
    }
}

void mpack_write_u16(mpack_writer_t* writer, uint16_t value) {
    mpack_track_element_write(writer);
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
    mpack_track_element_write(writer);
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
    mpack_track_element_write(writer);
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

    mpack_track_element_write(writer);
    if (value >= -32) {
        mpack_write_native_u8(writer, 0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
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

    mpack_track_element_write(writer);
    if (value >= -32) {
        mpack_write_native_u8(writer, 0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
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

    mpack_track_element_write(writer);
    if (value >= -32) {
        mpack_write_native_u8(writer, 0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
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

    mpack_track_element_write(writer);
    if (value >= -32) {
        mpack_write_native_u8(writer, 0xe0 | (int8_t)value); // TODO: remove this (compatibility/1.1 difference?)
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
    mpack_track_element_write(writer);
    mpack_write_native_u8(writer, 0xc2 | (value ? 1 : 0));
}

void mpack_write_nil(mpack_writer_t* writer) {
    mpack_track_element_write(writer);
    mpack_write_native_u8(writer, 0xc0);
}

void mpack_write_float(mpack_writer_t* writer, float value) {
    mpack_track_element_write(writer);
    mpack_write_native_u8(writer, 0xca);
    mpack_write_native_float(writer, value);
}

void mpack_write_double(mpack_writer_t* writer, double value) {
    mpack_track_element_write(writer);
    mpack_write_native_u8(writer, 0xcb);
    mpack_write_native_double(writer, value);
}

#if MPACK_TRACKING
void mpack_finish_array(mpack_writer_t* writer) {
    mpack_finish_track(writer, mpack_type_array);
}

void mpack_finish_map(mpack_writer_t* writer) {
    mpack_finish_track(writer, mpack_type_map);
}

void mpack_finish_str(mpack_writer_t* writer) {
    mpack_finish_track(writer, mpack_type_str);
}

void mpack_finish_bin(mpack_writer_t* writer) {
    mpack_finish_track(writer, mpack_type_bin);
}

void mpack_finish_ext(mpack_writer_t* writer) {
    mpack_finish_track(writer, mpack_type_ext);
}
#endif

void mpack_start_array(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_track_element_write(writer);
    if (count <= 15) {
        mpack_write_native_u8(writer, 0x90 | (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xdc);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xdd);
        mpack_write_native_u32(writer, count);
    }

    mpack_writer_add_track(writer, mpack_type_array, count);
}

void mpack_start_map(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_track_element_write(writer);
    if (count <= 15) {
        mpack_write_native_u8(writer, 0x80 | (uint8_t)count);
    } else if (count <= UINT16_MAX) {
        mpack_write_native_u8(writer, 0xde);
        mpack_write_native_u16(writer, (uint16_t)count);
    } else {
        mpack_write_native_u8(writer, 0xdf);
        mpack_write_native_u32(writer, count);
    }

    mpack_writer_add_track(writer, mpack_type_map, count * 2);
}

void mpack_start_str(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_track_element_write(writer);
    if (count <= 31) {
        mpack_write_native_u8(writer, 0xa0 | (uint8_t)count);
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

    mpack_writer_add_track(writer, mpack_type_str, count);
}

void mpack_start_bin(mpack_writer_t* writer, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    mpack_track_element_write(writer);
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

    mpack_writer_add_track(writer, mpack_type_bin, count);
}

void mpack_start_ext(mpack_writer_t* writer, int8_t exttype, uint32_t count) {
    if (writer->error != mpack_ok)
        return;

    // TODO: fail if compatibility mode

    mpack_track_element_write(writer);
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

    mpack_writer_add_track(writer, mpack_type_ext, count);
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
    mpack_track_bytes_written(writer, count);
    mpack_write_native(writer, data, count);
}

void mpack_write_cstr(mpack_writer_t* writer, const char* str) {
    size_t len = strlen(str);
    if (len > UINT32_MAX)
        mpack_writer_flag_error(writer, mpack_error_invalid);
    mpack_write_str(writer, str, len);
}

#endif

