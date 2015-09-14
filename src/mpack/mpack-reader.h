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
 * Declares the core MPack Tag Reader.
 */

#ifndef MPACK_READER_H
#define MPACK_READER_H 1

#include "mpack-common.h"

#ifdef MPACK_READER

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MPACK_READ_TRACKING
struct mpack_track_t;
#endif

/**
 * @defgroup reader Core Reader API
 *
 * The MPack Core Reader API contains functions for imperatively reading
 * dynamically typed data from a MessagePack stream. This forms the basis
 * of the Expect and Node APIs.
 *
 * @{
 */

/**
 * A buffered MessagePack decoder.
 *
 * The decoder wraps an existing buffer and, optionally, a fill function.
 * This allows efficiently decoding data from existing memory buffers, files,
 * streams, etc.
 *
 * All read operations are synchronous; they will block until the
 * requested data is fully read, or an error occurs.
 *
 * This structure is opaque; its fields should not be accessed outside
 * of MPack.
 */
typedef struct mpack_reader_t mpack_reader_t;

/**
 * The mpack reader's fill function. It should fill the buffer as
 * much as possible, returning the number of bytes put into the buffer.
 *
 * In case of error, it should flag an appropriate error on the reader.
 */
typedef size_t (*mpack_fill_t)(mpack_reader_t* reader, char* buffer, size_t count);

/**
 * A teardown function to be called when the reader is destroyed.
 */
typedef void (*mpack_reader_teardown_t)(mpack_reader_t* reader);

struct mpack_reader_t {
    mpack_fill_t fill;                /* Function to read bytes into the buffer */
    mpack_reader_teardown_t teardown; /* Function to teardown the context on destroy */
    void* context;                    /* Context for reader callbacks */

    char* buffer;       /* Byte buffer */
    size_t size;        /* Size of the buffer, or zero if it's const */
    size_t left;        /* How many bytes are left in the buffer */
    size_t pos;         /* Position within the buffer */
    mpack_error_t error;  /* Error state */

    #ifdef MPACK_SETJMP
    /* Optional jump target in case of error (pointer because it's
     * very large and may be unused) */
    jmp_buf* jump_env;
    #endif

    #ifdef MPACK_READ_TRACKING
    mpack_track_t track; /* Stack of map/array/str/bin/ext reads */
    #endif
};

#ifdef MPACK_SETJMP

/**
 * @hideinitializer
 *
 * Registers a jump target in case of error.
 *
 * If the reader is in an error state, 1 is returned when this is called. Otherwise
 * 0 is returned when this is called, and when the first error occurs, control flow
 * will jump to the point where this was called, resuming as though it returned 1.
 * This ensures an error handling block runs exactly once in case of error.
 *
 * A reader that jumps still needs to be destroyed. You must call
 * mpack_reader_destroy() in your jump handler after getting the final error state.
 *
 * The argument may be evaluated multiple times.
 *
 * @returns 0 if the reader is not in an error state; 1 if and when an error occurs.
 * @see mpack_reader_destroy()
 */
#define MPACK_READER_SETJMP(reader)                                        \
    (mpack_assert((reader)->jump_env == NULL, "already have a jump set!"), \
    ((reader)->error != mpack_ok) ? 1 :                                    \
        !((reader)->jump_env = (jmp_buf*)MPACK_MALLOC(sizeof(jmp_buf))) ?  \
            ((reader)->error = mpack_error_memory, 1) :                    \
            (setjmp(*(reader)->jump_env)))

/**
 * Clears a jump target. Subsequent read errors will not cause the reader to
 * jump.
 */
static inline void mpack_reader_clearjmp(mpack_reader_t* reader) {
    if (reader->jump_env)
        MPACK_FREE(reader->jump_env);
    reader->jump_env = NULL;
}
#endif

/**
 * Initializes an mpack reader with the given buffer. The reader does
 * not assume ownership of the buffer, but the buffer must be writeable
 * if a fill function will be used to refill it.
 *
 * @param reader The MPack reader.
 * @param buffer The buffer with which to read mpack data.
 * @param size The size of the buffer.
 * @param count The number of bytes already in the buffer.
 */
void mpack_reader_init(mpack_reader_t* reader, char* buffer, size_t size, size_t count);

/**
 * Initializes an mpack reader directly into an error state. Use this if you
 * are writing a wrapper to mpack_reader_init() which can fail its setup.
 */
void mpack_reader_init_error(mpack_reader_t* reader, mpack_error_t error);

/**
 * Initializes an mpack reader to parse a pre-loaded contiguous chunk of data. The
 * reader does not assume ownership of the data.
 *
 * @param reader The MPack reader.
 * @param data The data to parse.
 * @param count The number of bytes pointed to by data.
 */
void mpack_reader_init_data(mpack_reader_t* reader, const char* data, size_t count);

#ifdef MPACK_STDIO
/**
 * Initializes an mpack reader that reads from a file.
 */
void mpack_reader_init_file(mpack_reader_t* reader, const char* filename);
#endif

/**
 * @def mpack_reader_init_stack(reader)
 * @hideinitializer
 *
 * Initializes an mpack reader using stack space as a buffer. A fill function
 * should be added to the reader to fill the buffer.
 *
 * @see mpack_reader_set_fill
 */

/** @cond */
#define mpack_reader_init_stack_line_ex(line, reader) \
    char mpack_buf_##line[MPACK_STACK_SIZE]; \
    mpack_reader_init((reader), mpack_buf_##line, sizeof(mpack_buf_##line), 0)

#define mpack_reader_init_stack_line(line, reader) \
    mpack_reader_init_stack_line_ex(line, reader)
/** @endcond */

#define mpack_reader_init_stack(reader) \
    mpack_reader_init_stack_line(__LINE__, (reader))

/**
 * Cleans up the mpack reader, ensuring that all compound elements
 * have been completely read. Returns the final error state of the
 * reader.
 *
 * This will assert in tracking mode if the reader has any incomplete
 * reads. If you want to cancel reading in the middle of a compound
 * element and don't care about the rest of the document, call
 * mpack_reader_destroy_cancel() instead.
 *
 * @see mpack_reader_destroy_cancel()
 */
mpack_error_t mpack_reader_destroy(mpack_reader_t* reader);

/**
 * Cleans up the mpack reader, discarding any open reads.
 *
 * This should be used if you decide to cancel reading in the middle
 * of the document.
 */
void mpack_reader_destroy_cancel(mpack_reader_t* reader);

/**
 * Sets the custom pointer to pass to the reader callbacks, such as fill
 * or teardown.
 *
 * @param reader The MPack reader.
 * @param context User data to pass to the reader callbacks.
 */
static inline void mpack_reader_set_context(mpack_reader_t* reader, void* context) {
    reader->context = context;
}

/**
 * Sets the fill function to refill the data buffer when it runs out of data.
 *
 * If no fill function is used, trying to read past the end of the
 * buffer will result in mpack_error_io.
 *
 * This should normally be used with mpack_reader_set_context() to register
 * a custom pointer to pass to the fill function.
 *
 * @param reader The MPack reader.
 * @param fill The function to fetch additional data into the buffer.
 */
static inline void mpack_reader_set_fill(mpack_reader_t* reader, mpack_fill_t fill) {
    mpack_assert(reader->size != 0, "cannot use fill function without a writeable buffer!");
    reader->fill = fill;
}

/**
 * Sets the teardown function to call when the reader is destroyed.
 *
 * This should normally be used with mpack_reader_set_context() to register
 * a custom pointer to pass to the teardown function.
 *
 * @param reader The MPack reader.
 * @param teardown The function to call when the reader is destroyed.
 */
static inline void mpack_reader_set_teardown(mpack_reader_t* reader, mpack_reader_teardown_t teardown) {
    reader->teardown = teardown;
}

/**
 * Queries the error state of the MPack reader.
 *
 * If a reader is in an error state, you should discard all data since the
 * last time the error flag was checked. The error flag cannot be cleared.
 */
static inline mpack_error_t mpack_reader_error(mpack_reader_t* reader) {
    return reader->error;
}

/**
 * Places the reader in the given error state, jumping if a jump target is set.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you read it.
 *
 * If the reader is already in an error state, this call is ignored and no jump
 * is performed.
 */
void mpack_reader_flag_error(mpack_reader_t* reader, mpack_error_t error);

/**
 * Places the reader in the given error state if the given error is not mpack_ok,
 * returning the resulting error state of the reader.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you read it.
 *
 * If the given error is mpack_ok or if the reader is already in an error state,
 * this call is ignored and the actual error state of the reader is returned.
 */
static inline mpack_error_t mpack_reader_flag_if_error(mpack_reader_t* reader, mpack_error_t error) {
    if (error != mpack_ok)
        mpack_reader_flag_error(reader, error);
    return mpack_reader_error(reader);
}

/**
 * Returns bytes left in the reader's buffer.
 *
 * If you are done reading MessagePack data but there is other interesting data
 * following it, the reader may have buffered too much data. The number of bytes
 * remaining in the buffer and a pointer to the position of those bytes can be
 * queried here.
 *
 * If you know the length of the mpack chunk beforehand, it's better to instead
 * have your fill function limit the data it reads so that the reader does not
 * have extra data. In this case you can simply check that this returns zero.
 *
 * @param reader The MPack reader from which to query remaining data.
 * @param data [out] A pointer to the remaining data, or NULL.
 * @return The number of bytes remaining in the buffer.
 */
size_t mpack_reader_remaining(mpack_reader_t* reader, const char** data);

/**
 * Reads a MessagePack object header (an MPack tag.)
 *
 * If an error occurs, the mpack_reader_t is placed in an error state, a
 * longjmp is performed (if set), and a nil tag is returned. If the reader
 * is already in an error state, a nil tag is returned.
 *
 * If the type is compound (i.e. is a map, array, string, binary or
 * extension type), additional reads are required to get the actual data,
 * and the corresponding done function (or cancel) should be called when
 * done.
 *
 * Note that maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 * 
 * @see mpack_read_bytes()
 * @see mpack_done_array()
 * @see mpack_done_map()
 * @see mpack_done_str()
 * @see mpack_done_bin()
 * @see mpack_done_ext()
 * @see mpack_cancel()
 */
mpack_tag_t mpack_read_tag(mpack_reader_t* reader);

/**
 * Skips bytes from the underlying stream. This is used only to
 * skip the contents of a string, binary blob or extension object.
 */
void mpack_skip_bytes(mpack_reader_t* reader, size_t count);

/**
 * Reads bytes from a string, binary blob or extension object.
 */
void mpack_read_bytes(mpack_reader_t* reader, char* p, size_t count);

/**
 * Reads bytes from a string, binary blob or extension object in-place in
 * the buffer. This can be used to avoid copying the data.
 *
 * The returned pointer is invalidated the next time the reader's fill
 * function is called, or when the buffer is destroyed.
 *
 * The size requested must be at most the buffer size. If the requested size is
 * larger than the buffer size, mpack_error_too_big is raised and the
 * return value is undefined.
 *
 * The reader will move data around in the buffer if needed to ensure that
 * the pointer can always be returned, so it is unlikely to be faster unless
 * count is very small compared to the buffer size. If you need to check
 * whether a small size is reasonable (for example you intend to handle small and
 * large sizes differently), you can call mpack_should_read_bytes_inplace().
 *
 * As with all read functions, the return value is undefined if the reader
 * is in an error state.
 *
 * @see mpack_should_read_bytes_inplace()
 */
const char* mpack_read_bytes_inplace(mpack_reader_t* reader, size_t count);

/**
 * Returns true if it's a good idea to read the given number of bytes
 * in-place.
 *
 * If the read will be larger than some small fraction of the buffer size,
 * this will return false to avoid shuffling too much data back and forth
 * in the buffer.
 *
 * Use this if you're expecting arbitrary size data, and you want to read
 * in-place where possible but will fall back to a normal read if the data
 * is too large.
 *
 * @see mpack_read_bytes_inplace()
 */
static inline bool mpack_should_read_bytes_inplace(mpack_reader_t* reader, size_t count) {
    return (reader->size == 0 || count > reader->size / 8);
}

#ifdef MPACK_READ_TRACKING
/**
 * Finishes reading an array.
 *
 * This will track reads to ensure that the correct number of elements are read.
 */
void mpack_done_array(mpack_reader_t* reader);

/**
 * @fn mpack_done_map(mpack_reader_t* reader)
 *
 * Finishes reading a map.
 *
 * This will track reads to ensure that the correct number of elements are read.
 */
void mpack_done_map(mpack_reader_t* reader);

/**
 * @fn mpack_done_str(mpack_reader_t* reader)
 *
 * Finishes reading a string.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_str(mpack_reader_t* reader);

/**
 * @fn mpack_done_bin(mpack_reader_t* reader)
 *
 * Finishes reading a binary data blob.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_bin(mpack_reader_t* reader);

/**
 * @fn mpack_done_ext(mpack_reader_t* reader)
 *
 * Finishes reading an extended type binary data blob.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_ext(mpack_reader_t* reader);

/**
 * Finishes reading the given type.
 *
 * This will track reads to ensure that the correct number of elements
 * or bytes are read.
 */
void mpack_done_type(mpack_reader_t* reader, mpack_type_t type);
#else
static inline void mpack_done_array(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_map(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_str(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_bin(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_ext(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_type(mpack_reader_t* reader, mpack_type_t type) {MPACK_UNUSED(reader); MPACK_UNUSED(type);}
#endif

/**
 * Reads and discards the next object. This will read and discard all
 * contained data as well if it is a compound type.
 */
void mpack_discard(mpack_reader_t* reader);

#if defined(MPACK_DEBUG) && defined(MPACK_STDIO) && defined(MPACK_SETJMP) && !defined(MPACK_NO_PRINT)
/*! Converts a chunk of messagepack to JSON and pretty-prints it to stdout. */
void mpack_debug_print(const char* data, int len);
#endif

/**
 * @}
 */



#ifdef MPACK_INTERNAL

void mpack_read_native_big(mpack_reader_t* reader, char* p, size_t count);

// Reads count bytes into p, deferring to mpack_read_native_big() if more
// bytes are needed than are available in the buffer.
static inline void mpack_read_native(mpack_reader_t* reader, char* p, size_t count) {
    if (count > reader->left) {
        mpack_read_native_big(reader, p, count);
    } else {
        mpack_memcpy(p, reader->buffer + reader->pos, count);
        reader->pos += count;
        reader->left -= count;
    }
}

// Reads native bytes with jump disabled. This allows mpack reader functions
// to hold an allocated buffer and read native data into it without leaking it.
static inline void mpack_read_native_nojump(mpack_reader_t* reader, char* p, size_t count) {
    #ifdef MPACK_SETJMP
    jmp_buf* jump_env = reader->jump_env;
    reader->jump_env = NULL;
    #endif
    mpack_read_native(reader, p, count);
    #ifdef MPACK_SETJMP
    reader->jump_env = jump_env;
    #endif
}

MPACK_ALWAYS_INLINE uint8_t mpack_read_native_u8(mpack_reader_t* reader) {
    if (reader->left >= sizeof(uint8_t)) {
        uint8_t ret = mpack_load_native_u8(reader->buffer + reader->pos);
        reader->pos += sizeof(uint8_t);
        reader->left -= sizeof(uint8_t);
        return ret;
    }

    char c[sizeof(uint8_t)];
    mpack_read_native_big(reader, c, sizeof(c));
    return mpack_load_native_u8(c);
}

MPACK_ALWAYS_INLINE uint16_t mpack_read_native_u16(mpack_reader_t* reader) {
    if (reader->left >= sizeof(uint16_t)) {
        uint16_t ret = mpack_load_native_u16(reader->buffer + reader->pos);
        reader->pos += sizeof(uint16_t);
        reader->left -= sizeof(uint16_t);
        return ret;
    }

    char c[sizeof(uint16_t)];
    mpack_read_native_big(reader, c, sizeof(c));
    return mpack_load_native_u16(c);
}

MPACK_ALWAYS_INLINE uint32_t mpack_read_native_u32(mpack_reader_t* reader) {
    if (reader->left >= sizeof(uint32_t)) {
        uint32_t ret = mpack_load_native_u32(reader->buffer + reader->pos);
        reader->pos += sizeof(uint32_t);
        reader->left -= sizeof(uint32_t);
        return ret;
    }

    char c[sizeof(uint32_t)];
    mpack_read_native_big(reader, c, sizeof(c));
    return mpack_load_native_u32(c);
}

MPACK_ALWAYS_INLINE uint64_t mpack_read_native_u64(mpack_reader_t* reader) {
    if (reader->left >= sizeof(uint64_t)) {
        uint64_t ret = mpack_load_native_u64(reader->buffer + reader->pos);
        reader->pos += sizeof(uint64_t);
        reader->left -= sizeof(uint64_t);
        return ret;
    }

    char c[sizeof(uint64_t)];
    mpack_read_native_big(reader, c, sizeof(c));
    return mpack_load_native_u64(c);
}

MPACK_ALWAYS_INLINE int8_t  mpack_read_native_i8  (mpack_reader_t* reader) {return (int8_t) mpack_read_native_u8  (reader);}
MPACK_ALWAYS_INLINE int16_t mpack_read_native_i16 (mpack_reader_t* reader) {return (int16_t)mpack_read_native_u16 (reader);}
MPACK_ALWAYS_INLINE int32_t mpack_read_native_i32 (mpack_reader_t* reader) {return (int32_t)mpack_read_native_u32 (reader);}
MPACK_ALWAYS_INLINE int64_t mpack_read_native_i64 (mpack_reader_t* reader) {return (int64_t)mpack_read_native_u64 (reader);}

MPACK_ALWAYS_INLINE float mpack_read_native_float(mpack_reader_t* reader) {
    union {
        float f;
        uint32_t i;
    } u;
    u.i = mpack_read_native_u32(reader);
    return u.f;
}

MPACK_ALWAYS_INLINE double mpack_read_native_double(mpack_reader_t* reader) {
    union {
        double d;
        uint64_t i;
    } u;
    u.i = mpack_read_native_u64(reader);
    return u.d;
}

#ifdef MPACK_READ_TRACKING
#define MPACK_READER_TRACK(reader, error) mpack_reader_flag_if_error((reader), (error))
#else
#define MPACK_READER_TRACK(reader, error) (MPACK_UNUSED(reader), mpack_ok)
#endif

static inline mpack_error_t mpack_reader_track_element(mpack_reader_t* reader) {
    return MPACK_READER_TRACK(reader, mpack_track_element(&reader->track, true));
}

static inline mpack_error_t mpack_reader_track_bytes(mpack_reader_t* reader, uint64_t count) {
    MPACK_UNUSED(count);
    return MPACK_READER_TRACK(reader, mpack_track_bytes(&reader->track, true, count));
}

#endif



#ifdef __cplusplus
}
#endif

#endif
#endif

