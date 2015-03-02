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

#if MPACK_READER

#ifdef __cplusplus
extern "C" {
#endif

#if MPACK_TRACKING
/* Tracks the read state of compound elements (map, array, string, bin, ext) */
typedef struct mpack_reader_track_t mpack_reader_track_t;
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
 */
typedef size_t (*mpack_fill_t)(void* context, char* buffer, size_t count);

#if MPACK_STDIO
/**
 * An implementation of the mpack fill function which wraps the
 * standard stdio fread() function. Pass the FILE handle as the context.
 *
 * If an error occurs, mpack_error_io will be raised. You can use
 * feof() and ferror() to determine what error occurred.
 */
size_t mpack_fread(void* context, char* buffer, size_t count);
#endif

struct mpack_reader_t {
    mpack_fill_t fill;  /* Function to read bytes into the buffer */
    void* context;      /* Context for the reader function */
    char* buffer;       /* Byte buffer */
    size_t size;        /* Size of the buffer */
    size_t left;        /* How many bytes are left in the buffer */
    size_t pos;         /* Position within the buffer */
    mpack_error_t error;  /* Error state */

    #if MPACK_SETJMP
    bool jump;          /* Whether to longjmp on error */
    jmp_buf jump_env;   /* Where to jump */
    #endif

    #if MPACK_TRACKING
    mpack_reader_track_t* track; /* Stack of map/array/str/bin/ext reads */
    #endif
};

#if MPACK_SETJMP

/**
 * @hideinitializer
 *
 * Registers a jump target in case of error.
 *
 * If the reader is in an error state, 1 is returned when called. Otherwise 0 is
 * returned when called, and when the first error occurs, control flow will jump
 * to the point where MPACK_READER_SETJMP() was called, resuming as though it
 * returned 1. This ensures an error handling block runs exactly once in case of
 * error.
 *
 * The argument may be evaluated multiple times.
 *
 * @returns 0 if the reader is not in an error state; 1 if and when an error occurs.
 */
#define MPACK_READER_SETJMP(reader) (((reader)->error == mpack_ok) ? \
    ((reader)->jump = true, setjmp((reader)->jump_env)) : 1)

/**
 * Clears a jump target. Subsequent read errors will not cause the reader to
 * jump.
 */
static inline void mpack_reader_clearjmp(mpack_reader_t* reader) {
    reader->jump = false;
}
#endif

/**
 * Initializes an mpack reader with the given buffer and fill function. The reader
 * does not assume ownership of the buffer.
 *
 * For an in-memory buffer, the fill function may be NULL. In this case trying to
 * read past the end of the buffer will result in mpack_error_io.
 */
static inline void mpack_reader_init(
        mpack_reader_t* reader, /*!< The MessagePack reader. */
        mpack_fill_t fill,      /*!< The function to fetch additional data into the buffer. */
        void* context,          /*!< User data to pass to the fill function. */
        char* buffer,           /*!< The buffer with which to read mpack data. */
        size_t size,            /*!< The size of the buffer. */
        size_t count            /*!< The number of bytes already in the buffer. */
) {
    memset(reader, 0, sizeof(*reader));
    reader->fill = fill;
    reader->context = context;
    reader->buffer = buffer;
    reader->size = size;
    reader->left = count;
}

/**
 * Initializes an mpack reader with an in-memory buffer. The reader does not
 * assume ownership of the buffer.
 */
static inline void mpack_reader_init_buffer(
        mpack_reader_t* reader, /*!< The MessagePack reader. */
        const char* buffer,     /*!< The buffer from which to read mpack data. */
        size_t count            /*!< The number of bytes in the buffer. */
) {
    memset(reader, 0, sizeof(*reader));
    reader->left = count;
    
    /* unfortunately we have to cast away the const to store the buffer, */
    /* but we won't be modifying it because there's no fill function. */
    reader->buffer = (char*)buffer;
}

/**
 * @def mpack_reader_init_stack(reader, fill, context)
 * @hideinitializer
 *
 * Initializes an mpack reader using stack space with the given fill
 * function and context.
 */

#define mpack_reader_init_stack_line_ex(line, reader, fill, context) \
    char mpack_buf_##line[MPACK_STACK_SIZE]; \
    mpack_reader_init((reader), (fill), (context), mpack_buf_##line, sizeof(mpack_buf_##line), 0)

#define mpack_reader_init_stack_line(line, reader, fill, context) \
    mpack_reader_init_stack_line_ex(line, reader, fill, context)

#define mpack_reader_init_stack(reader, fill, context) \
    mpack_reader_init_stack_line(__LINE__, (reader), (fill), (context))

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
 * TODO change all destroy to destroy
 */
mpack_error_t mpack_reader_destroy(mpack_reader_t* reader);

/**
 * Cleans up the mpack reader, discarding any open reads.
 *
 * This should be used if you decide to cancel reading in the middle
 * of the document.
 */
mpack_error_t mpack_reader_destroy_cancel(mpack_reader_t* reader);

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
 * Queries the error state of the MPack reader.
 *
 * If a reader is in an error state, you should discard all data since the
 * last time the error flag was checked. The error flag cannot be cleared.
 */
static inline mpack_error_t mpack_reader_error(mpack_reader_t* reader) {
    return reader->error;
}

/**
 * Reads a MessagePack object header (an MPack tag.)
 *
 * If an error occurs, the mpack_reader_t is placed in an error state, a
 * longjmp is performed (if set), and the return value is undefined.
 * If the reader is already in an error state, the return value
 * is undefined.
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
 * count is very small compared to the buffer size.
 *
 * As with all read functions, the return value is undefined if the reader
 * is in an error state.
 */
const char* mpack_read_bytes_inplace(mpack_reader_t* reader, size_t count);

#if MPACK_TRACKING
/**
 * Finishes reading an array.
 *
 * In release mode, this is a no-op. However if a debug build is used, this
 * will track reads to ensure that the correct number of elements are read.
 */
void mpack_done_array(mpack_reader_t* reader);

/**
 * @fn mpack_done_map(mpack_reader_t* reader)
 *
 * Finishes reading a map.
 *
 * In release mode, this is a no-op. However if a debug build is used, this
 * will track reads to ensure that the correct number of elements are read.
 *
 * You must call mpack_done_map() when done.
 */
void mpack_done_map(mpack_reader_t* reader);

/**
 * @fn mpack_done_str(mpack_reader_t* reader)
 *
 * Finishes reading a string.
 *
 * In release mode, this is a no-op. However if a debug build is used, this
 * will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_str(mpack_reader_t* reader);

/**
 * @fn mpack_done_bin(mpack_reader_t* reader)
 *
 * Finishes reading a binary data blob.
 *
 * In release mode, this is a no-op. However if a debug build is used, this
 * will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_bin(mpack_reader_t* reader);

/**
 * @fn mpack_done_ext(mpack_reader_t* reader)
 *
 * Finishes reading an extended binary data blob.
 *
 * In release mode, this is a no-op. However if a debug build is used, this
 * will track reads to ensure that the correct number of bytes are read.
 */
void mpack_done_ext(mpack_reader_t* reader);

#else
static inline void mpack_done_array(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_map(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_str(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_bin(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
static inline void mpack_done_ext(mpack_reader_t* reader) {MPACK_UNUSED(reader);}
#endif

/**
 * Reads and discards the next object. This will read and discard all
 * contained data as well if it is a compound type.
 */
void mpack_discard(mpack_reader_t* reader);

#if MPACK_DEBUG && MPACK_STDIO && MPACK_SETJMP
/*! Converts a chunk of messagepack to JSON and pretty-prints it to stdout. */
void mpack_debug_print(const char* data, int len);
#endif

/**
 * @}
 */

/*
 * the remaining functions are used by the expect API. they are undocumented
 * internal-only functions (comments are around their implementation)
 */

void mpack_read_native(mpack_reader_t* reader, char* p, size_t count);

static inline uint8_t mpack_read_native_u8(mpack_reader_t* reader) {
    char c[sizeof(uint8_t)];
    mpack_read_native(reader, c, sizeof(c));
    return (uint8_t)c[0];
}

void mpack_read_native_nojump(mpack_reader_t* reader, char* p, size_t count);
float mpack_read_native_float(mpack_reader_t* reader);
double mpack_read_native_double(mpack_reader_t* reader);

#if MPACK_TRACKING
void mpack_track_read(mpack_reader_t* reader, bool bytes, uint64_t count);
#endif

static inline void mpack_track_element_read(mpack_reader_t* reader) {
    MPACK_UNUSED(reader);
    #if MPACK_TRACKING
    mpack_track_read(reader, false, 1);
    #endif
}

static inline void mpack_track_bytes_read(mpack_reader_t* reader, uint64_t count) {
    MPACK_UNUSED(reader);
    MPACK_UNUSED(count);
    #if MPACK_TRACKING
    mpack_track_read(reader, true, count);
    #endif
}

#ifdef __cplusplus
}
#endif

#endif
#endif

