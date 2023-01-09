/*
 * Copyright (c) 2015-2021 Nicholas Fraser and the MPack authors
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

MPACK_SILENCE_WARNINGS_BEGIN

#if MPACK_WRITER

#if MPACK_BUILDER
static void mpack_builder_flush(mpack_writer_t* writer);
#endif

#if MPACK_WRITE_TRACKING
static void mpack_writer_flag_if_error(mpack_writer_t* writer, mpack_error_t error) {
    if (error != mpack_ok)
        mpack_writer_flag_error(writer, error);
}

void mpack_writer_track_push(mpack_writer_t* writer, mpack_type_t type, uint32_t count) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_push(&writer->track, type, count));
}

void mpack_writer_track_push_builder(mpack_writer_t* writer, mpack_type_t type) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_push_builder(&writer->track, type));
}

void mpack_writer_track_pop(mpack_writer_t* writer, mpack_type_t type) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_pop(&writer->track, type));
}

void mpack_writer_track_pop_builder(mpack_writer_t* writer, mpack_type_t type) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_pop_builder(&writer->track, type));
}

void mpack_writer_track_bytes(mpack_writer_t* writer, size_t count) {
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_bytes(&writer->track, false, count));
}
#endif

// This should probably be renamed. It's not solely used for tracking.
static inline void mpack_writer_track_element(mpack_writer_t* writer) {
    (void)writer;

    #if MPACK_WRITE_TRACKING
    if (writer->error == mpack_ok)
        mpack_writer_flag_if_error(writer, mpack_track_element(&writer->track, false));
    #endif

    #if MPACK_BUILDER
    if (writer->builder.current_build != NULL) {
        mpack_build_t* build = writer->builder.current_build;
        // We only track this write if it's not nested within another non-build
        // map or array.
        if (build->nested_compound_elements == 0) {
            if (build->type != mpack_type_map) {
                ++build->count;
                mpack_log("adding element to build %p, now %" PRIu32 " elements\n", (void*)build, build->count);
            } else if (build->key_needs_value) {
                build->key_needs_value = false;
                ++build->count;
            } else {
                build->key_needs_value = true;
            }
        }
    }
    #endif
}

static void mpack_writer_clear(mpack_writer_t* writer) {
    #if MPACK_COMPATIBILITY
    writer->version = mpack_version_current;
    #endif
    writer->flush = NULL;
    writer->error_fn = NULL;
    writer->teardown = NULL;
    writer->context = NULL;

    writer->buffer = NULL;
    writer->position = NULL;
    writer->end = NULL;
    writer->error = mpack_ok;

    #if MPACK_WRITE_TRACKING
    mpack_memset(&writer->track, 0, sizeof(writer->track));
    #endif

    #if MPACK_BUILDER
    writer->builder.current_build = NULL;
    writer->builder.latest_build = NULL;
    writer->builder.current_page = NULL;
    writer->builder.pages = NULL;
    writer->builder.stash_buffer = NULL;
    writer->builder.stash_position = NULL;
    writer->builder.stash_end = NULL;
    #endif
}

void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size) {
    mpack_assert(buffer != NULL, "cannot initialize writer with empty buffer");
    mpack_writer_clear(writer);
    writer->buffer = buffer;
    writer->position = buffer;
    writer->end = writer->buffer + size;

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
    MPACK_STATIC_ASSERT(31 + MPACK_TAG_SIZE_FIXSTR >= MPACK_WRITER_MINIMUM_BUFFER_SIZE,
            "minimum buffer size must fit the largest possible fixstr!");

    if (mpack_writer_buffer_size(writer) < MPACK_WRITER_MINIMUM_BUFFER_SIZE) {
        mpack_break("buffer size is %i, but minimum buffer size for flush is %i",
                (int)mpack_writer_buffer_size(writer), MPACK_WRITER_MINIMUM_BUFFER_SIZE);
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

static char* mpack_writer_get_reserved(mpack_writer_t* writer) {
    // This is in a separate function in order to avoid false strict aliasing
    // warnings. We aren't actually violating strict aliasing (the reserved
    // space is only ever dereferenced as an mpack_growable_writer_t.)
    return (char*)writer->reserved;
}

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
        if (mpack_writer_buffer_used(writer) == count)
            return;

        // otherwise leave the data in the buffer and just grow
        writer->position = writer->buffer + count;
        count = 0;
    }

    size_t used = mpack_writer_buffer_used(writer);
    size_t size = mpack_writer_buffer_size(writer);

    mpack_log("flush size %i used %i data %p buffer %p\n",
            (int)count, (int)used, data, writer->buffer);

    mpack_assert(data == writer->buffer || used + count > size,
            "extra flush for %i but there is %i space left in the buffer! (%i/%i)",
            (int)count, (int)mpack_writer_buffer_left(writer), (int)used, (int)size);

    // grow to fit the data
    // TODO: this really needs to correctly test for overflow
    size_t new_size = size * 2;
    while (new_size < used + count)
        new_size *= 2;

    mpack_log("flush growing buffer size from %i to %i\n", (int)size, (int)new_size);

    // grow the buffer
    char* new_buffer = (char*)mpack_realloc(writer->buffer, used, new_size);
    if (new_buffer == NULL) {
        mpack_writer_flag_error(writer, mpack_error_memory);
        return;
    }
    writer->position = new_buffer + used;
    writer->buffer = new_buffer;
    writer->end = writer->buffer + new_size;

    // append the extra data
    if (count > 0) {
        mpack_memcpy(writer->position, data, count);
        writer->position += count;
    }

    mpack_log("new buffer %p, used %i\n", new_buffer, (int)mpack_writer_buffer_used(writer));
}

static void mpack_growable_writer_teardown(mpack_writer_t* writer) {
    mpack_growable_writer_t* growable_writer = (mpack_growable_writer_t*)mpack_writer_get_reserved(writer);

    if (mpack_writer_error(writer) == mpack_ok) {

        // shrink the buffer to an appropriate size if the data is
        // much smaller than the buffer
        if (mpack_writer_buffer_used(writer) < mpack_writer_buffer_size(writer) / 2) {
            size_t used = mpack_writer_buffer_used(writer);

            // We always return a non-null pointer that must be freed, even if
            // nothing was written. malloc() and realloc() do not necessarily
            // do this so we enforce it ourselves.
            size_t size = (used != 0) ? used : 1;

            char* buffer = (char*)mpack_realloc(writer->buffer, used, size);
            if (!buffer) {
                MPACK_FREE(writer->buffer);
                mpack_writer_flag_error(writer, mpack_error_memory);
                return;
            }
            writer->buffer = buffer;
            writer->end = (writer->position = writer->buffer + used);
        }

        *growable_writer->target_data = writer->buffer;
        *growable_writer->target_size = mpack_writer_buffer_used(writer);
        writer->buffer = NULL;

    } else if (writer->buffer) {
        MPACK_FREE(writer->buffer);
        writer->buffer = NULL;
    }

    writer->context = NULL;
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
    MPACK_FREE(writer->buffer);
    writer->buffer = NULL;
    writer->context = NULL;
}

static void mpack_file_writer_teardown_close(mpack_writer_t* writer) {
    FILE* file = (FILE*)writer->context;

    if (file) {
        int ret = fclose(file);
        if (ret != 0)
            mpack_writer_flag_error(writer, mpack_error_io);
    }

    mpack_file_writer_teardown(writer);
}

void mpack_writer_init_stdfile(mpack_writer_t* writer, FILE* file, bool close_when_done) {
    mpack_assert(file != NULL, "file is NULL");

    size_t capacity = MPACK_BUFFER_SIZE;
    char* buffer = (char*)MPACK_MALLOC(capacity);
    if (buffer == NULL) {
        mpack_writer_init_error(writer, mpack_error_memory);
        if (close_when_done) {
            fclose(file);
        }
        return;
    }

    mpack_writer_init(writer, buffer, capacity);
    mpack_writer_set_context(writer, file);
    mpack_writer_set_flush(writer, mpack_file_writer_flush);
    mpack_writer_set_teardown(writer, close_when_done ?
            mpack_file_writer_teardown_close :
            mpack_file_writer_teardown);
}

void mpack_writer_init_filename(mpack_writer_t* writer, const char* filename) {
    mpack_assert(filename != NULL, "filename is NULL");

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        mpack_writer_init_error(writer, mpack_error_io);
        return;
    }

    mpack_writer_init_stdfile(writer, file, true);
}
#endif

void mpack_writer_flag_error(mpack_writer_t* writer, mpack_error_t error) {
    mpack_log("writer %p setting error %i: %s\n", (void*)writer, (int)error, mpack_error_to_string(error));

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
    size_t used = mpack_writer_buffer_used(writer);
    writer->position = writer->buffer;
    writer->flush(writer, writer->buffer, used);
}

void mpack_writer_flush_message(mpack_writer_t* writer) {
    if (writer->error != mpack_ok)
        return;

    #if MPACK_WRITE_TRACKING
    // You cannot flush while there are elements open.
    mpack_writer_flag_if_error(writer, mpack_track_check_empty(&writer->track));
    if (writer->error != mpack_ok)
        return;
    #endif

    #if MPACK_BUILDER
    if (writer->builder.current_build != NULL) {
        mpack_break("cannot call mpack_writer_flush_message() while there are elements open!");
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }
    #endif

    if (writer->flush == NULL) {
        mpack_break("cannot call mpack_writer_flush_message() without a flush function!");
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    if (mpack_writer_buffer_used(writer) > 0)
        mpack_writer_flush_unchecked(writer);
}

// Ensures there are at least count bytes free in the buffer. This
// will flag an error if the flush function fails to make enough
// room in the buffer.
MPACK_NOINLINE static bool mpack_writer_ensure(mpack_writer_t* writer, size_t count) {
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

    #if MPACK_BUILDER
    // if we have a build in progress, we just ask the builder for a page.
    // either it will have space for a tag, or it will flag a memory error.
    if (writer->builder.current_build != NULL) {
        mpack_builder_flush(writer);
        return mpack_writer_error(writer) == mpack_ok;
    }
    #endif

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
MPACK_NOINLINE static void mpack_write_native_straddle(mpack_writer_t* writer, const char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (mpack_writer_error(writer) != mpack_ok)
        return;
    mpack_log("big write for %i bytes from %p, %i space left in buffer\n",
            (int)count, p, (int)mpack_writer_buffer_left(writer));
    mpack_assert(count > mpack_writer_buffer_left(writer),
            "big write requested for %i bytes, but there is %i available "
            "space in buffer. should have called mpack_write_native() instead",
            (int)count, (int)(mpack_writer_buffer_left(writer)));

    #if MPACK_BUILDER
    // if we have a build in progress, we can't flush. we need to copy all
    // bytes into as many build buffer pages as it takes.
    if (writer->builder.current_build != NULL) {
        while (true) {
            size_t step = (size_t)(writer->end - writer->position);
            if (step > count)
                step = count;
            mpack_memcpy(writer->position, p, step);
            writer->position += step;
            p += step;
            count -= step;

            if (count == 0)
                return;

            mpack_builder_flush(writer);
            if (mpack_writer_error(writer) != mpack_ok)
                return;
            mpack_assert(writer->position != writer->end);
        }
    }
    #endif

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
    if (count > mpack_writer_buffer_left(writer)) {
        writer->flush(writer, p, count);
        if (mpack_writer_error(writer) != mpack_ok)
            return;
    } else {
        mpack_memcpy(writer->position, p, count);
        writer->position += count;
    }
}

// Writes encoded bytes to the buffer, flushing if necessary.
MPACK_STATIC_INLINE void mpack_write_native(mpack_writer_t* writer, const char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (mpack_writer_buffer_left(writer) < count) {
        mpack_write_native_straddle(writer, p, count);
    } else {
        mpack_memcpy(writer->position, p, count);
        writer->position += count;
    }
}

mpack_error_t mpack_writer_destroy(mpack_writer_t* writer) {

    // clean up tracking, asserting if we're not already in an error state
    #if MPACK_WRITE_TRACKING
    mpack_track_destroy(&writer->track, writer->error != mpack_ok);
    #endif

    #if MPACK_BUILDER
    mpack_builder_t* builder = &writer->builder;
    if (builder->current_build != NULL) {
        // A builder is open!

        // Flag an error, if there's not already an error. You can only skip
        // closing any open compound types if a write error occurred. If there
        // wasn't already an error, it's a bug, which will assert in debug.
        if (mpack_writer_error(writer) == mpack_ok) {
            mpack_break("writer cannot be destroyed with an incomplete builder unless "
                    "an error was flagged!");
            mpack_writer_flag_error(writer, mpack_error_bug);
        }

        // Free any remaining builder pages
        mpack_builder_page_t* page = builder->pages;
        #if MPACK_BUILDER_INTERNAL_STORAGE
        mpack_assert(page == (mpack_builder_page_t*)builder->internal);
        page = page->next;
        #endif
        while (page != NULL) {
            mpack_builder_page_t* next = page->next;
            MPACK_FREE(page);
            page = next;
        }

        // Restore the stashed pointers. The teardown function may need to free
        // them (e.g. mpack_growable_writer_teardown().)
        writer->buffer = builder->stash_buffer;
        writer->position = builder->stash_position;
        writer->end = builder->stash_end;

        // Note: It's not necessary to clean up the current_build or other
        // pointers at this point because we're guaranteed to be in an error
        // state already so a user error callback can't longjmp out. This
        // destroy function will complete no matter what so it doesn't matter
        // what junk is left in the writer.
    }
    #endif

    // flush any outstanding data
    if (mpack_writer_error(writer) == mpack_ok && mpack_writer_buffer_used(writer) != 0 && writer->flush != NULL) {
        writer->flush(writer, writer->buffer, mpack_writer_buffer_used(writer));
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
        case mpack_type_missing:
            mpack_break("cannot write a missing value!");
            mpack_writer_flag_error(writer, mpack_error_bug);
            return;

        case mpack_type_nil:    mpack_write_nil   (writer);            return;
        case mpack_type_bool:   mpack_write_bool  (writer, value.v.b); return;
        case mpack_type_int:    mpack_write_int   (writer, value.v.i); return;
        case mpack_type_uint:   mpack_write_uint  (writer, value.v.u); return;

        case mpack_type_float:
            #if MPACK_FLOAT
            mpack_write_float
            #else
            mpack_write_raw_float
            #endif
                (writer, value.v.f);
            return;
        case mpack_type_double:
            #if MPACK_DOUBLE
            mpack_write_double
            #else
            mpack_write_raw_double
            #endif
                (writer, value.v.d);
            return;

        case mpack_type_str: mpack_start_str(writer, value.v.l); return;
        case mpack_type_bin: mpack_start_bin(writer, value.v.l); return;

        #if MPACK_EXTENSIONS
        case mpack_type_ext:
            mpack_start_ext(writer, mpack_tag_ext_exttype(&value), mpack_tag_ext_length(&value));
            return;
        #endif

        case mpack_type_array: mpack_start_array(writer, value.v.n); return;
        case mpack_type_map:   mpack_start_map(writer, value.v.n);   return;
    }

    mpack_break("unrecognized type %i", (int)value.type);
    mpack_writer_flag_error(writer, mpack_error_bug);
}

MPACK_STATIC_INLINE void mpack_write_byte_element(mpack_writer_t* writer, char value) {
    mpack_writer_track_element(writer);
    if (MPACK_LIKELY(mpack_writer_buffer_left(writer) >= 1) || mpack_writer_ensure(writer, 1))
        *(writer->position++) = value;
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
    mpack_assert(value > MPACK_UINT8_MAX);
    mpack_store_u8(p, 0xcd);
    mpack_store_u16(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_u32(char* p, uint32_t value) {
    mpack_assert(value > MPACK_UINT16_MAX);
    mpack_store_u8(p, 0xce);
    mpack_store_u32(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_u64(char* p, uint64_t value) {
    mpack_assert(value > MPACK_UINT32_MAX);
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
    mpack_assert(value < MPACK_INT8_MIN);
    mpack_store_u8(p, 0xd1);
    mpack_store_i16(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_i32(char* p, int32_t value) {
    mpack_assert(value < MPACK_INT16_MIN);
    mpack_store_u8(p, 0xd2);
    mpack_store_i32(p + 1, value);
}

MPACK_STATIC_INLINE void mpack_encode_i64(char* p, int64_t value) {
    mpack_assert(value < MPACK_INT32_MIN);
    mpack_store_u8(p, 0xd3);
    mpack_store_i64(p + 1, value);
}

#if MPACK_FLOAT
MPACK_STATIC_INLINE void mpack_encode_float(char* p, float value) {
    mpack_store_u8(p, 0xca);
    mpack_store_float(p + 1, value);
}
#else
MPACK_STATIC_INLINE void mpack_encode_raw_float(char* p, uint32_t value) {
    mpack_store_u8(p, 0xca);
    mpack_store_u32(p + 1, value);
}
#endif

#if MPACK_DOUBLE
MPACK_STATIC_INLINE void mpack_encode_double(char* p, double value) {
    mpack_store_u8(p, 0xcb);
    mpack_store_double(p + 1, value);
}
#else
MPACK_STATIC_INLINE void mpack_encode_raw_double(char* p, uint64_t value) {
    mpack_store_u8(p, 0xcb);
    mpack_store_u64(p + 1, value);
}
#endif

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
    mpack_assert(count > MPACK_UINT16_MAX);
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
    mpack_assert(count > MPACK_UINT16_MAX);
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
    // allow count to be in the range [32, MPACK_UINT8_MAX].
    mpack_assert(count > 31);
    mpack_store_u8(p, 0xda);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_str32(char* p, uint32_t count) {
    mpack_assert(count > MPACK_UINT16_MAX);
    mpack_store_u8(p, 0xdb);
    mpack_store_u32(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin8(char* p, uint8_t count) {
    mpack_store_u8(p, 0xc4);
    mpack_store_u8(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin16(char* p, uint16_t count) {
    mpack_assert(count > MPACK_UINT8_MAX);
    mpack_store_u8(p, 0xc5);
    mpack_store_u16(p + 1, count);
}

MPACK_STATIC_INLINE void mpack_encode_bin32(char* p, uint32_t count) {
    mpack_assert(count > MPACK_UINT16_MAX);
    mpack_store_u8(p, 0xc6);
    mpack_store_u32(p + 1, count);
}

#if MPACK_EXTENSIONS
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
    mpack_assert(count > MPACK_UINT8_MAX);
    mpack_store_u8(p, 0xc8);
    mpack_store_u16(p + 1, count);
    mpack_store_i8(p + 3, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_ext32(char* p, int8_t exttype, uint32_t count) {
    mpack_assert(count > MPACK_UINT16_MAX);
    mpack_store_u8(p, 0xc9);
    mpack_store_u32(p + 1, count);
    mpack_store_i8(p + 5, exttype);
}

MPACK_STATIC_INLINE void mpack_encode_timestamp_4(char* p, uint32_t seconds) {
    mpack_encode_fixext4(p, MPACK_EXTTYPE_TIMESTAMP);
    mpack_store_u32(p + MPACK_TAG_SIZE_FIXEXT4, seconds);
}

MPACK_STATIC_INLINE void mpack_encode_timestamp_8(char* p, int64_t seconds, uint32_t nanoseconds) {
    mpack_assert(nanoseconds <= MPACK_TIMESTAMP_NANOSECONDS_MAX);
    mpack_encode_fixext8(p, MPACK_EXTTYPE_TIMESTAMP);
    uint64_t encoded = ((uint64_t)nanoseconds << 34) | (uint64_t)seconds;
    mpack_store_u64(p + MPACK_TAG_SIZE_FIXEXT8, encoded);
}

MPACK_STATIC_INLINE void mpack_encode_timestamp_12(char* p, int64_t seconds, uint32_t nanoseconds) {
    mpack_assert(nanoseconds <= MPACK_TIMESTAMP_NANOSECONDS_MAX);
    mpack_encode_ext8(p, MPACK_EXTTYPE_TIMESTAMP, 12);
    mpack_store_u32(p + MPACK_TAG_SIZE_EXT8, nanoseconds);
    mpack_store_i64(p + MPACK_TAG_SIZE_EXT8 + 4, seconds);
}
#endif



/*
 * Write functions
 */

// This is a macro wrapper to the encode functions to encode
// directly into the buffer. If mpack_writer_ensure() fails
// it will flag an error so we don't have to do anything.
#define MPACK_WRITE_ENCODED(encode_fn, size, ...) do {                                                 \
    if (MPACK_LIKELY(mpack_writer_buffer_left(writer) >= size) || mpack_writer_ensure(writer, size)) { \
        MPACK_EXPAND(encode_fn(writer->position, __VA_ARGS__));                                        \
        writer->position += size;                                                                      \
    }                                                                                                  \
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
    } else if (value <= MPACK_UINT8_MAX) {
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
    } else if (value <= MPACK_UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
    } else if (value <= MPACK_UINT16_MAX) {
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
    } else if (value <= MPACK_UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
    } else if (value <= MPACK_UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
    } else if (value <= MPACK_UINT32_MAX) {
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
        } else if (value <= MPACK_UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        }
    } else if (value >= MPACK_INT8_MIN) {
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
        } else if (value <= MPACK_UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else if (value <= MPACK_UINT16_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, (uint32_t)value);
        }
    } else if (value >= MPACK_INT8_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    } else if (value >= MPACK_INT16_MIN) {
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
        } else if (value <= MPACK_UINT8_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u8, MPACK_TAG_SIZE_U8, (uint8_t)value);
        } else if (value <= MPACK_UINT16_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u16, MPACK_TAG_SIZE_U16, (uint16_t)value);
        } else if (value <= MPACK_UINT32_MAX) {
            MPACK_WRITE_ENCODED(mpack_encode_u32, MPACK_TAG_SIZE_U32, (uint32_t)value);
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_u64, MPACK_TAG_SIZE_U64, (uint64_t)value);
        }
        #endif
    } else if (value >= MPACK_INT8_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i8, MPACK_TAG_SIZE_I8, (int8_t)value);
    } else if (value >= MPACK_INT16_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i16, MPACK_TAG_SIZE_I16, (int16_t)value);
    } else if (value >= MPACK_INT32_MIN) {
        MPACK_WRITE_ENCODED(mpack_encode_i32, MPACK_TAG_SIZE_I32, (int32_t)value);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_i64, MPACK_TAG_SIZE_I64, value);
    }
}

#if MPACK_FLOAT
void mpack_write_float(mpack_writer_t* writer, float value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_float, MPACK_TAG_SIZE_FLOAT, value);
}
#else
void mpack_write_raw_float(mpack_writer_t* writer, uint32_t value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_raw_float, MPACK_TAG_SIZE_FLOAT, value);
}
#endif

#if MPACK_DOUBLE
void mpack_write_double(mpack_writer_t* writer, double value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_double, MPACK_TAG_SIZE_DOUBLE, value);
}
#else
void mpack_write_raw_double(mpack_writer_t* writer, uint64_t value) {
    mpack_writer_track_element(writer);
    MPACK_WRITE_ENCODED(mpack_encode_raw_double, MPACK_TAG_SIZE_DOUBLE, value);
}
#endif

#if MPACK_EXTENSIONS
void mpack_write_timestamp(mpack_writer_t* writer, int64_t seconds, uint32_t nanoseconds) {
    #if MPACK_COMPATIBILITY
    if (writer->version <= mpack_version_v4) {
        mpack_break("Timestamps require spec version v5 or later. This writer is in v%i mode.", (int)writer->version);
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }
    #endif

    if (nanoseconds > MPACK_TIMESTAMP_NANOSECONDS_MAX) {
        mpack_break("timestamp nanoseconds out of bounds: %" PRIu32 , nanoseconds);
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    mpack_writer_track_element(writer);

    if (seconds < 0 || seconds >= (MPACK_INT64_C(1) << 34)) {
        MPACK_WRITE_ENCODED(mpack_encode_timestamp_12, MPACK_EXT_SIZE_TIMESTAMP12, seconds, nanoseconds);
    } else if (seconds > MPACK_UINT32_MAX || nanoseconds > 0) {
        MPACK_WRITE_ENCODED(mpack_encode_timestamp_8, MPACK_EXT_SIZE_TIMESTAMP8, seconds, nanoseconds);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_timestamp_4, MPACK_EXT_SIZE_TIMESTAMP4, (uint32_t)seconds);
    }
}
#endif

static void mpack_write_array_notrack(mpack_writer_t* writer, uint32_t count) {
    if (count <= 15) {
        MPACK_WRITE_ENCODED(mpack_encode_fixarray, MPACK_TAG_SIZE_FIXARRAY, (uint8_t)count);
    } else if (count <= MPACK_UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_array16, MPACK_TAG_SIZE_ARRAY16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_array32, MPACK_TAG_SIZE_ARRAY32, (uint32_t)count);
    }
}

static void mpack_write_map_notrack(mpack_writer_t* writer, uint32_t count) {
    if (count <= 15) {
        MPACK_WRITE_ENCODED(mpack_encode_fixmap, MPACK_TAG_SIZE_FIXMAP, (uint8_t)count);
    } else if (count <= MPACK_UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_map16, MPACK_TAG_SIZE_MAP16, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_map32, MPACK_TAG_SIZE_MAP32, (uint32_t)count);
    }
}

void mpack_start_array(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);
    mpack_write_array_notrack(writer, count);
    mpack_writer_track_push(writer, mpack_type_array, count);
    mpack_builder_compound_push(writer);
}

void mpack_start_map(mpack_writer_t* writer, uint32_t count) {
    mpack_writer_track_element(writer);
    mpack_write_map_notrack(writer, count);
    mpack_writer_track_push(writer, mpack_type_map, count);
    mpack_builder_compound_push(writer);
}

static void mpack_start_str_notrack(mpack_writer_t* writer, uint32_t count) {
    if (count <= 31) {
        MPACK_WRITE_ENCODED(mpack_encode_fixstr, MPACK_TAG_SIZE_FIXSTR, (uint8_t)count);

    // str8 is only supported in v5 or later.
    } else if (count <= MPACK_UINT8_MAX
            #if MPACK_COMPATIBILITY
            && writer->version >= mpack_version_v5
            #endif
            ) {
        MPACK_WRITE_ENCODED(mpack_encode_str8, MPACK_TAG_SIZE_STR8, (uint8_t)count);

    } else if (count <= MPACK_UINT16_MAX) {
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

    if (count <= MPACK_UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_bin8, MPACK_TAG_SIZE_BIN8, (uint8_t)count);
    } else if (count <= MPACK_UINT16_MAX) {
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

#if MPACK_EXTENSIONS
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
    } else if (count <= MPACK_UINT8_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_ext8, MPACK_TAG_SIZE_EXT8, exttype, (uint8_t)count);
    } else if (count <= MPACK_UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_ext16, MPACK_TAG_SIZE_EXT16, exttype, (uint16_t)count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_ext32, MPACK_TAG_SIZE_EXT32, exttype, (uint32_t)count);
    }

    mpack_writer_track_push(writer, mpack_type_ext, count);
}
#endif



/*
 * Compound helpers and other functions
 */

void mpack_write_str(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_assert(count == 0 || data != NULL, "data for string of length %i is NULL", (int)count);

    #if MPACK_OPTIMIZE_FOR_SIZE
    mpack_writer_track_element(writer);
    mpack_start_str_notrack(writer, count);
    mpack_write_native(writer, data, count);
    #else

    mpack_writer_track_element(writer);

    if (count <= 31) {
        // The minimum buffer size when using a flush function is guaranteed to
        // fit the largest possible fixstr.
        size_t size = count + MPACK_TAG_SIZE_FIXSTR;
        if (MPACK_LIKELY(mpack_writer_buffer_left(writer) >= size) || mpack_writer_ensure(writer, size)) {
            char* MPACK_RESTRICT p = writer->position;
            mpack_encode_fixstr(p, (uint8_t)count);
            mpack_memcpy(p + MPACK_TAG_SIZE_FIXSTR, data, count);
            writer->position += count + MPACK_TAG_SIZE_FIXSTR;
        }
        return;
    }

    if (count <= MPACK_UINT8_MAX
            #if MPACK_COMPATIBILITY
            && writer->version >= mpack_version_v5
            #endif
            ) {
        if (count + MPACK_TAG_SIZE_STR8 <= mpack_writer_buffer_left(writer)) {
            char* MPACK_RESTRICT p = writer->position;
            mpack_encode_str8(p, (uint8_t)count);
            mpack_memcpy(p + MPACK_TAG_SIZE_STR8, data, count);
            writer->position += count + MPACK_TAG_SIZE_STR8;
        } else {
            MPACK_WRITE_ENCODED(mpack_encode_str8, MPACK_TAG_SIZE_STR8, (uint8_t)count);
            mpack_write_native(writer, data, count);
        }
        return;
    }

    // str16 and str32 are likely to be a significant fraction of the buffer
    // size, so we don't bother with a combined space check in order to
    // minimize code size.
    if (count <= MPACK_UINT16_MAX) {
        MPACK_WRITE_ENCODED(mpack_encode_str16, MPACK_TAG_SIZE_STR16, (uint16_t)count);
        mpack_write_native(writer, data, count);
    } else {
        MPACK_WRITE_ENCODED(mpack_encode_str32, MPACK_TAG_SIZE_STR32, (uint32_t)count);
        mpack_write_native(writer, data, count);
    }

    #endif
}

void mpack_write_bin(mpack_writer_t* writer, const char* data, uint32_t count) {
    mpack_assert(count == 0 || data != NULL, "data pointer for bin of %i bytes is NULL", (int)count);
    mpack_start_bin(writer, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_bin(writer);
}

#if MPACK_EXTENSIONS
void mpack_write_ext(mpack_writer_t* writer, int8_t exttype, const char* data, uint32_t count) {
    mpack_assert(count == 0 || data != NULL, "data pointer for ext of type %i and %i bytes is NULL", exttype, (int)count);
    mpack_start_ext(writer, exttype, count);
    mpack_write_bytes(writer, data, count);
    mpack_finish_ext(writer);
}
#endif

void mpack_write_bytes(mpack_writer_t* writer, const char* data, size_t count) {
    mpack_assert(count == 0 || data != NULL, "data pointer for %i bytes is NULL", (int)count);
    mpack_writer_track_bytes(writer, count);
    mpack_write_native(writer, data, count);
}

void mpack_write_cstr(mpack_writer_t* writer, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr pointer is NULL");
    size_t length = mpack_strlen(cstr);
    if (length > MPACK_UINT32_MAX)
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
    mpack_assert(length == 0 || str != NULL, "data for string of length %i is NULL", (int)length);
    if (!mpack_utf8_check(str, length)) {
        mpack_writer_flag_error(writer, mpack_error_invalid);
        return;
    }
    mpack_write_str(writer, str, length);
}

void mpack_write_utf8_cstr(mpack_writer_t* writer, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr pointer is NULL");
    size_t length = mpack_strlen(cstr);
    if (length > MPACK_UINT32_MAX) {
        mpack_writer_flag_error(writer, mpack_error_invalid);
        return;
    }
    mpack_write_utf8(writer, cstr, (uint32_t)length);
}

void mpack_write_utf8_cstr_or_nil(mpack_writer_t* writer, const char* cstr) {
    if (cstr)
        mpack_write_utf8_cstr(writer, cstr);
    else
        mpack_write_nil(writer);
}

/*
 * Builder implementation
 *
 * When a writer is in build mode, it diverts writes to an internal growable
 * buffer. All elements other than builder start tags are encoded as normal
 * into the builder buffer (even nested maps and arrays of known size, e.g.
 * `mpack_start_array()`.) But for compound elements of unknown size, an
 * mpack_build_t is written to the buffer instead.
 *
 * The mpack_build_t tracks everything needed to re-constitute the final
 * message once all sizes are known. When the last build element is completed,
 * the builder resolves the build by walking through the builds, outputting the
 * final encoded tag, and copying everything in between to the writer's true
 * buffer.
 *
 * To make things extra complicated, the builder buffer is not contiguous. It's
 * allocated in pages, where the first page may be an internal page in the
 * writer. But, each mpack_build_t must itself be contiguous and aligned
 * properly within the buffer. This means bytes can be skipped (and wasted)
 * before the builds or at the end of pages.
 *
 * To keep track of this, builds store both their element count and the number
 * of encoded bytes that follow, and pages store the number of bytes used. As
 * elements are written, each element adds to the count in the current open
 * build, and the number of bytes written adds to the current page and the byte
 * count in the last started build (whether or not it is completed.)
 */

#if MPACK_BUILDER

#ifdef MPACK_ALIGNOF
    #define MPACK_BUILD_ALIGNMENT MPACK_ALIGNOF(mpack_build_t)
#else
    // without alignof, we just align to the greater of size_t, void* and uint64_t.
    // (we do this even though we don't have uint64_t in it in case we add it later.)
    #define MPACK_BUILD_ALIGNMENT_MAX(x, y) ((x) > (y) ? (x) : (y))
    #define MPACK_BUILD_ALIGNMENT (MPACK_BUILD_ALIGNMENT_MAX(sizeof(void*), \
                MPACK_BUILD_ALIGNMENT_MAX(sizeof(size_t), sizeof(uint64_t))))
#endif

static inline void mpack_builder_check_sizes(mpack_writer_t* writer) {

    // We check internal and page sizes here so that we don't have to check
    // them again. A new page with a build in it will have a page header,
    // build, and minimum space for a tag. This will perform horribly and waste
    // tons of memory if the page size is small, so you're best off just
    // sticking with the defaults.
    //
    // These are all known at compile time, so if they are large
    // enough this function should trivially optimize to a no-op.

    #if MPACK_BUILDER_INTERNAL_STORAGE
    // make sure the internal storage is big enough to be useful
    MPACK_STATIC_ASSERT(MPACK_BUILDER_INTERNAL_STORAGE_SIZE >= (sizeof(mpack_builder_page_t) +
            sizeof(mpack_build_t) + MPACK_WRITER_MINIMUM_BUFFER_SIZE),
            "MPACK_BUILDER_INTERNAL_STORAGE_SIZE is too small to be useful!");
    if (MPACK_BUILDER_INTERNAL_STORAGE_SIZE < (sizeof(mpack_builder_page_t) +
            sizeof(mpack_build_t) + MPACK_WRITER_MINIMUM_BUFFER_SIZE))
    {
        mpack_break("MPACK_BUILDER_INTERNAL_STORAGE_SIZE is too small to be useful!");
        mpack_writer_flag_error(writer, mpack_error_bug);
    }
    #endif

    // make sure the builder page size is big enough to be useful
    MPACK_STATIC_ASSERT(MPACK_BUILDER_PAGE_SIZE >= (sizeof(mpack_builder_page_t) +
            sizeof(mpack_build_t) + MPACK_WRITER_MINIMUM_BUFFER_SIZE),
            "MPACK_BUILDER_PAGE_SIZE is too small to be useful!");
    if (MPACK_BUILDER_PAGE_SIZE < (sizeof(mpack_builder_page_t) +
            sizeof(mpack_build_t) + MPACK_WRITER_MINIMUM_BUFFER_SIZE))
    {
        mpack_break("MPACK_BUILDER_PAGE_SIZE is too small to be useful!");
        mpack_writer_flag_error(writer, mpack_error_bug);
    }
}

static inline size_t mpack_builder_page_size(mpack_writer_t* writer, mpack_builder_page_t* page) {
    #if MPACK_BUILDER_INTERNAL_STORAGE
    if ((char*)page == writer->builder.internal)
        return sizeof(writer->builder.internal);
    #else
    (void)writer;
    (void)page;
    #endif
    return MPACK_BUILDER_PAGE_SIZE;
}

static inline size_t mpack_builder_align_build(size_t bytes_used) {
    size_t offset = bytes_used;
    offset += MPACK_BUILD_ALIGNMENT - 1;
    offset -= offset % MPACK_BUILD_ALIGNMENT;
    mpack_log("aligned %zi to %zi\n", bytes_used, offset);
    return offset;
}

static inline void mpack_builder_free_page(mpack_writer_t* writer, mpack_builder_page_t* page) {
    mpack_log("freeing page %p\n", (void*)page);
    #if MPACK_BUILDER_INTERNAL_STORAGE
    if ((char*)page == writer->builder.internal)
        return;
    #else
    (void)writer;
    #endif
    MPACK_FREE(page);
}

static inline size_t mpack_builder_page_remaining(mpack_writer_t* writer, mpack_builder_page_t* page) {
    return mpack_builder_page_size(writer, page) - page->bytes_used;
}

static void mpack_builder_configure_buffer(mpack_writer_t* writer) {
    if (mpack_writer_error(writer) != mpack_ok)
        return;
    mpack_builder_t* builder = &writer->builder;

    mpack_builder_page_t* page = builder->current_page;
    mpack_assert(page != NULL, "page is null??");

    // This diverts the writer into the remainder of the current page of our
    // build buffer.
    writer->buffer = (char*)page + page->bytes_used;
    writer->position = (char*)page + page->bytes_used;
    writer->end = (char*)page + mpack_builder_page_size(writer, page);
    mpack_log("configuring buffer from %p to %p\n", (void*)writer->position, (void*)writer->end);
}

static void mpack_builder_add_page(mpack_writer_t* writer) {
    mpack_builder_t* builder = &writer->builder;
    mpack_assert(writer->error == mpack_ok);

    mpack_log("adding a page.\n");
    mpack_builder_page_t* page = (mpack_builder_page_t*)MPACK_MALLOC(MPACK_BUILDER_PAGE_SIZE);
    if (page == NULL) {
        mpack_writer_flag_error(writer, mpack_error_memory);
        return;
    }

    page->next = NULL;
    page->bytes_used = sizeof(mpack_builder_page_t);
    builder->current_page->next = page;
    builder->current_page = page;
}

// Checks how many bytes the writer wrote to the page, adding it to the page's
// bytes_used. This must be followed up with mpack_builder_configure_buffer()
// (after adding a new page, build, etc) to reset the writer's buffer pointers.
static void mpack_builder_apply_writes(mpack_writer_t* writer) {
    mpack_assert(writer->error == mpack_ok);
    mpack_builder_t* builder = &writer->builder;
    mpack_log("latest build is %p\n", (void*)builder->latest_build);

    // The difference between buffer and current is the number of bytes that
    // were written to the page.
    size_t bytes_written = (size_t)(writer->position - writer->buffer);
    mpack_log("applying write of %zi bytes to build %p\n", bytes_written, (void*)builder->latest_build);

    mpack_assert(builder->current_page != NULL);
    mpack_assert(builder->latest_build != NULL);
    builder->current_page->bytes_used += bytes_written;
    builder->latest_build->bytes += bytes_written;
    mpack_log("latest build %p now has %zi bytes\n", (void*)builder->latest_build, builder->latest_build->bytes);
}

static void mpack_builder_flush(mpack_writer_t* writer) {
    mpack_assert(writer->error == mpack_ok);
    mpack_builder_apply_writes(writer);
    mpack_builder_add_page(writer);
    mpack_builder_configure_buffer(writer);
}

MPACK_NOINLINE static void mpack_builder_begin(mpack_writer_t* writer) {
    mpack_builder_t* builder = &writer->builder;
    mpack_assert(writer->error == mpack_ok);
    mpack_assert(builder->current_build == NULL);
    mpack_assert(builder->latest_build == NULL);
    mpack_assert(builder->pages == NULL);

    // If this is the first build, we need to stash the real buffer backing our
    // writer. We'll be diverting the writer to our build buffer.
    builder->stash_buffer = writer->buffer;
    builder->stash_position = writer->position;
    builder->stash_end = writer->end;

    mpack_builder_page_t* page;

    // we've checked that both these sizes are large enough above.
    #if MPACK_BUILDER_INTERNAL_STORAGE
    page = (mpack_builder_page_t*)builder->internal;
    mpack_log("beginning builder with internal storage %p\n", (void*)page);
    #else
    page = (mpack_builder_page_t*)MPACK_MALLOC(MPACK_BUILDER_PAGE_SIZE);
    if (page == NULL) {
        mpack_writer_flag_error(writer, mpack_error_memory);
        return;
    }
    mpack_log("beginning builder with allocated page %p\n", (void*)page);
    #endif

    page->next = NULL;
    page->bytes_used = sizeof(mpack_builder_page_t);
    builder->pages = page;
    builder->current_page = page;
}

static void mpack_builder_build(mpack_writer_t* writer, mpack_type_t type) {
    mpack_builder_check_sizes(writer);
    if (mpack_writer_error(writer) != mpack_ok)
        return;

    mpack_writer_track_element(writer);
    mpack_writer_track_push_builder(writer, type);

    mpack_builder_t* builder = &writer->builder;

    if (builder->current_build == NULL) {
        mpack_builder_begin(writer);
    } else {
        mpack_builder_apply_writes(writer);
    }
    if (mpack_writer_error(writer) != mpack_ok)
        return;

    // find aligned space for a new build. if there isn't enough space in the
    // current page, we discard the remaining space in it and allocate a new
    // page.
    size_t offset = mpack_builder_align_build(builder->current_page->bytes_used);
    if (offset + sizeof(mpack_build_t) > mpack_builder_page_size(writer, builder->current_page)) {
        mpack_log("not enough space for a build. %zi bytes used of %zi in this page\n",
                builder->current_page->bytes_used, mpack_builder_page_size(writer, builder->current_page));
        mpack_builder_add_page(writer);
        // there is always enough space in a fresh page.
        offset = mpack_builder_align_build(builder->current_page->bytes_used);
    }

    // allocate the build within the page. note that we don't keep track of the
    // space wasted due to the offset. instead the previous build has stored
    // how many bytes follow it, and we'll redo this offset calculation to find
    // this build after it.
    mpack_builder_page_t* page = builder->current_page;
    page->bytes_used = offset + sizeof(mpack_build_t);
    mpack_assert(page->bytes_used <= mpack_builder_page_size(writer, page));
    mpack_build_t* build = (mpack_build_t*)((char*)page + offset);
    mpack_log("created new build %p within page %p, which now has %zi bytes used\n",
            (void*)build, (void*)page, page->bytes_used);

    // configure the new build
    build->parent = builder->current_build;
    build->bytes = 0;
    build->count = 0;
    build->type = type;
    build->key_needs_value = false;
    build->nested_compound_elements = 0;

    mpack_log("setting current and latest build to new build %p\n", (void*)build);
    builder->current_build = build;
    builder->latest_build = build;

    // we always need to provide a buffer that meets the minimum buffer size.
    // if there isn't enough space, we discard the remaining space in the
    // current page and allocate a new one.
    if (mpack_builder_page_remaining(writer, page) < MPACK_WRITER_MINIMUM_BUFFER_SIZE) {
        mpack_log("less than minimum buffer size in current page. %zi bytes used of %zi in this page\n",
                builder->current_page->bytes_used, mpack_builder_page_size(writer, builder->current_page));
        mpack_builder_add_page(writer);
        if (mpack_writer_error(writer) != mpack_ok)
            return;
    }
    mpack_assert(mpack_builder_page_remaining(writer, builder->current_page) >= MPACK_WRITER_MINIMUM_BUFFER_SIZE);
    mpack_builder_configure_buffer(writer);
}

MPACK_NOINLINE
static void mpack_builder_resolve(mpack_writer_t* writer) {
    mpack_builder_t* builder = &writer->builder;

    // We should not have gotten here if we are in an error state. If an error
    // occurs with an open builder, the writer will free the open builder pages
    // when destroyed.
    mpack_assert(mpack_writer_error(writer) == mpack_ok, "can't resolve in error state!");

    // We don't want the user to longjmp out of any I/O errors while we are
    // walking the page list, so defer error callbacks to after we're done.
    mpack_writer_error_t error_fn = writer->error_fn;
    writer->error_fn = NULL;

    // The starting page is the internal storage (if we have it), otherwise
    // it's the first page in the array
    mpack_builder_page_t* page =
        #if MPACK_BUILDER_INTERNAL_STORAGE
        (mpack_builder_page_t*)builder->internal
        #else
        builder->pages
        #endif
        ;

    // We start by restoring the writer's original buffer so we can write the
    // data for real.
    writer->buffer = builder->stash_buffer;
    writer->position = builder->stash_position;
    writer->end = builder->stash_end;

    // We can also close out the build now.
    builder->current_build = NULL;
    builder->latest_build = NULL;
    builder->current_page = NULL;
    builder->pages = NULL;

    // the starting page always starts with the first build
    size_t offset = mpack_builder_align_build(sizeof(mpack_builder_page_t));
    mpack_build_t* build = (mpack_build_t*)((char*)page + offset);
    mpack_log("starting resolve with build %p in page %p\n", (void*)build, (void*)page);

    // encoded data immediately follows the build
    offset += sizeof(mpack_build_t);

    // Walk the list of builds, writing everything out in the buffer. Note that
    // we don't check for errors anywhere. The lower-level write functions will
    // all check for errors and do nothing after an error occurs. We need to
    // walk all pages anyway to free them, so there's not much point in
    // optimizing an error path at the expense of the normal path.
    while (true) {

        // write out the container tag
        mpack_log("writing out an %s with count %" PRIu32 " followed by %zi bytes\n",
                mpack_type_to_string(build->type), build->count, build->bytes);
        switch (build->type) {
            case mpack_type_map:
                mpack_write_map_notrack(writer, build->count);
                break;
            case mpack_type_array:
                mpack_write_array_notrack(writer, build->count);
                break;
            default:
                mpack_break("invalid type in builder?");
                mpack_writer_flag_error(writer, mpack_error_bug);
                return;
        }

        // figure out how many bytes follow this container. we're going to be
        // freeing pages as we write, so we need to be done with this build.
        size_t left = build->bytes;
        build = NULL;

        // write out all bytes following this container
        while (left > 0) {
            size_t bytes_used = page->bytes_used;
            if (offset < bytes_used) {
                size_t step = bytes_used - offset;
                if (step > left)
                    step = left;
                mpack_log("writing out %zi bytes starting at %p in page %p\n",
                        step, (void*)((char*)page + offset), (void*)page);
                mpack_write_native(writer, (char*)page + offset, step);
                offset += step;
                left -= step;
            }

            if (left == 0) {
                mpack_log("done writing bytes for this build\n");
                break;
            }

            // still need to write more bytes. free this page and jump to the
            // next one.
            mpack_builder_page_t* next_page = page->next;
            mpack_builder_free_page(writer, page);
            page = next_page;
            // bytes on the next page immediately follow the header.
            offset = sizeof(mpack_builder_page_t);
        }

        // now see if we can find another build.
        offset = mpack_builder_align_build(offset);
        if (offset + sizeof(mpack_build_t) > mpack_builder_page_size(writer, page)) {
            mpack_log("not enough room in this page for another build\n");
            mpack_builder_page_t* next_page = page->next;
            mpack_builder_free_page(writer, page);
            page = next_page;
            if (page == NULL) {
                mpack_log("no more pages\n");
                // there are no more pages. we're done.
                break;
            }
            offset = mpack_builder_align_build(sizeof(mpack_builder_page_t));
        }
        if (offset + sizeof(mpack_build_t) > page->bytes_used) {
            // there is no more data. we're done.
            mpack_log("no more data\n");
            mpack_builder_free_page(writer, page);
            break;
        }

        // we've found another build. loop around!
        build = (mpack_build_t*)((char*)page + offset);
        offset += sizeof(mpack_build_t);
        mpack_log("found build %p\n", (void*)build);
    }

    mpack_log("done resolve.\n");

    // We can now restore the error handler and call it if an error occurred.
    writer->error_fn = error_fn;
    if (writer->error_fn && mpack_writer_error(writer) != mpack_ok)
        writer->error_fn(writer, writer->error);
}

static void mpack_builder_complete(mpack_writer_t* writer, mpack_type_t type) {
    mpack_writer_track_pop_builder(writer, type);
    if (mpack_writer_error(writer) != mpack_ok)
        return;

    mpack_builder_t* builder = &writer->builder;
    mpack_assert(builder->current_build != NULL, "no build in progress!");
    mpack_assert(builder->latest_build != NULL, "missing latest build!");
    mpack_assert(builder->current_build->type == type, "completing wrong type!");
    mpack_log("completing build %p\n", (void*)builder->current_build);

    if (builder->current_build->key_needs_value) {
        mpack_break("an odd number of elements were written in a map!");
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    if (builder->current_build->nested_compound_elements != 0) {
        mpack_break("there is a nested unfinished non-build map or array in this build.");
        mpack_writer_flag_error(writer, mpack_error_bug);
        return;
    }

    // We need to apply whatever writes have been made to the current build
    // before popping it.
    mpack_builder_apply_writes(writer);

    // For a nested build, we just switch the current build back to its parent.
    if (builder->current_build->parent != NULL) {
        mpack_log("setting current build to parent build %p. latest is still %p.\n",
                (void*)builder->current_build->parent, (void*)builder->latest_build);
        builder->current_build = builder->current_build->parent;
        mpack_builder_configure_buffer(writer);
    } else {
        // We're completing the final build.
        mpack_builder_resolve(writer);
    }
}

void mpack_build_map(mpack_writer_t* writer) {
    mpack_builder_build(writer, mpack_type_map);
}

void mpack_build_array(mpack_writer_t* writer) {
    mpack_builder_build(writer, mpack_type_array);
}

void mpack_complete_map(mpack_writer_t* writer) {
    mpack_builder_complete(writer, mpack_type_map);
}

void mpack_complete_array(mpack_writer_t* writer) {
    mpack_builder_complete(writer, mpack_type_array);
}

#endif // MPACK_BUILDER
#endif // MPACK_WRITER

MPACK_SILENCE_WARNINGS_END
