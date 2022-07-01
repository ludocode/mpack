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

/**
 * @file
 *
 * Declares the MPack Writer.
 */

#ifndef MPACK_WRITER_H
#define MPACK_WRITER_H 1

#include "mpack-common.h"

#if MPACK_WRITER

MPACK_SILENCE_WARNINGS_BEGIN
MPACK_EXTERN_C_BEGIN

#if MPACK_WRITE_TRACKING
struct mpack_track_t;
#endif

/**
 * @defgroup writer Write API
 *
 * The MPack Write API encodes structured data of a fixed (hardcoded) schema to MessagePack.
 *
 * @{
 */

/**
 * @def MPACK_WRITER_MINIMUM_BUFFER_SIZE
 *
 * The minimum buffer size for a writer with a flush function.
 */
#define MPACK_WRITER_MINIMUM_BUFFER_SIZE 32

/**
 * A buffered MessagePack encoder.
 *
 * The encoder wraps an existing buffer and, optionally, a flush function.
 * This allows efficiently encoding to an in-memory buffer or to a stream.
 *
 * All write operations are synchronous; they will block until the
 * data is fully written, or an error occurs.
 */
typedef struct mpack_writer_t mpack_writer_t;

/**
 * The MPack writer's flush function to flush the buffer to the output stream.
 * It should flag an appropriate error on the writer if flushing fails (usually
 * mpack_error_io or mpack_error_memory.)
 *
 * The specified context for callbacks is at writer->context.
 */
typedef void (*mpack_writer_flush_t)(mpack_writer_t* writer, const char* buffer, size_t count);

/**
 * An error handler function to be called when an error is flagged on
 * the writer.
 *
 * The error handler will only be called once on the first error flagged;
 * any subsequent writes and errors are ignored, and the writer is
 * permanently in that error state.
 *
 * MPack is safe against non-local jumps out of error handler callbacks.
 * This means you are allowed to longjmp or throw an exception (in C++,
 * Objective-C, or with SEH) out of this callback.
 *
 * Bear in mind when using longjmp that local non-volatile variables that
 * have changed are undefined when setjmp() returns, so you can't put the
 * writer on the stack in the same activation frame as the setjmp without
 * declaring it volatile.
 *
 * You must still eventually destroy the writer. It is not destroyed
 * automatically when an error is flagged. It is safe to destroy the
 * writer within this error callback, but you will either need to perform
 * a non-local jump, or store something in your context to identify
 * that the writer is destroyed since any future accesses to it cause
 * undefined behavior.
 */
typedef void (*mpack_writer_error_t)(mpack_writer_t* writer, mpack_error_t error);

/**
 * A teardown function to be called when the writer is destroyed.
 */
typedef void (*mpack_writer_teardown_t)(mpack_writer_t* writer);

/* Hide internals from documentation */
/** @cond */

#if MPACK_BUILDER
/**
 * Build buffer pages form a linked list.
 *
 * They don't always fill up. If there is not enough space within them to write
 * a tag or place an mpack_build_t, a new page is allocated. For this reason
 * they store the number of used bytes.
 */
typedef struct mpack_builder_page_t {
    struct mpack_builder_page_t* next;
    size_t bytes_used;
} mpack_builder_page_t;

/**
 * Builds form a linked list of mpack_build_t, interleaved with their encoded
 * contents directly in the paged builder buffer.
 */
typedef struct mpack_build_t {
    //mpack_builder_page_t* page;
    struct mpack_build_t* parent;
    //struct mpack_build_t* next;

    size_t bytes; // number of bytes between this build and the next one
    uint32_t count; // number of elements (or key/value pairs) in this map/array
    mpack_type_t type;

    // depth of nested non-build compound elements within this
    // build.
    uint32_t nested_compound_elements;

    // indicates that a value still needs to be written for an already
    // written key. count is not incremented until both key and value are
    // written.
    bool key_needs_value;
} mpack_build_t;

/**
 * The builder state. This is stored within mpack_writer_t.
 */
typedef struct mpack_builder_t {
    mpack_build_t* current_build; // build which is accumulating elements
    mpack_build_t* latest_build; // build which is accumulating bytes
    mpack_builder_page_t* current_page;
    mpack_builder_page_t* pages;
    char* stash_buffer;
    char* stash_position;
    char* stash_end;
    #if MPACK_BUILDER_INTERNAL_STORAGE
    char internal[MPACK_BUILDER_INTERNAL_STORAGE_SIZE];
    #endif
} mpack_builder_t;
#endif

struct mpack_writer_t {
    #if MPACK_COMPATIBILITY
    mpack_version_t version;          /* Version of the MessagePack spec to write */
    #endif
    mpack_writer_flush_t flush;       /* Function to write bytes to the output stream */
    mpack_writer_error_t error_fn;    /* Function to call on error */
    mpack_writer_teardown_t teardown; /* Function to teardown the context on destroy */
    void* context;                    /* Context for writer callbacks */

    char* buffer;         /* Byte buffer */
    char* position;       /* Current position within the buffer */
    char* end;            /* The end of the buffer */
    mpack_error_t error;  /* Error state */

    #if MPACK_WRITE_TRACKING
    mpack_track_t track; /* Stack of map/array/str/bin/ext writes */
    #endif

    #ifdef MPACK_MALLOC
    /* Reserved. You can use this space to allocate a custom
     * context in order to reduce heap allocations. */
    void* reserved[2];
    #endif

    #if MPACK_BUILDER
    mpack_builder_t builder;
    #endif
};


#if MPACK_WRITE_TRACKING
void mpack_writer_track_push(mpack_writer_t* writer, mpack_type_t type, uint32_t count);
void mpack_writer_track_push_builder(mpack_writer_t* writer, mpack_type_t type);
void mpack_writer_track_pop(mpack_writer_t* writer, mpack_type_t type);
void mpack_writer_track_pop_builder(mpack_writer_t* writer, mpack_type_t type);
void mpack_writer_track_bytes(mpack_writer_t* writer, size_t count);
#else
MPACK_INLINE void mpack_writer_track_push(mpack_writer_t* writer, mpack_type_t type, uint32_t count) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(type);
    MPACK_UNUSED(count);
}
MPACK_INLINE void mpack_writer_track_push_builder(mpack_writer_t* writer, mpack_type_t type) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(type);
}
MPACK_INLINE void mpack_writer_track_pop(mpack_writer_t* writer, mpack_type_t type) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(type);
}
MPACK_INLINE void mpack_writer_track_pop_builder(mpack_writer_t* writer, mpack_type_t type) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(type);
}
MPACK_INLINE void mpack_writer_track_bytes(mpack_writer_t* writer, size_t count) {
    MPACK_UNUSED(writer);
    MPACK_UNUSED(count);
}
#endif

/** @endcond */

/**
 * @name Lifecycle Functions
 * @{
 */

/**
 * Initializes an MPack writer with the given buffer. The writer
 * does not assume ownership of the buffer.
 *
 * Trying to write past the end of the buffer will result in mpack_error_too_big
 * unless a flush function is set with mpack_writer_set_flush(). To use the data
 * without flushing, call mpack_writer_buffer_used() to determine the number of
 * bytes written.
 *
 * @param writer The MPack writer.
 * @param buffer The buffer into which to write MessagePack data.
 * @param size The size of the buffer.
 */
void mpack_writer_init(mpack_writer_t* writer, char* buffer, size_t size);

#ifdef MPACK_MALLOC
/**
 * Initializes an MPack writer using a growable buffer.
 *
 * The data is placed in the given data pointer if and when the writer
 * is destroyed without error. The data pointer is NULL during writing,
 * and will remain NULL if an error occurs.
 *
 * The allocated data must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 *
 * @throws mpack_error_memory if the buffer fails to grow when
 * flushing.
 *
 * @param writer The MPack writer.
 * @param data Where to place the allocated data.
 * @param size Where to write the size of the data.
 */
void mpack_writer_init_growable(mpack_writer_t* writer, char** data, size_t* size);
#endif

/**
 * Initializes an MPack writer directly into an error state. Use this if you
 * are writing a wrapper to mpack_writer_init() which can fail its setup.
 */
void mpack_writer_init_error(mpack_writer_t* writer, mpack_error_t error);

#if MPACK_STDIO
/**
 * Initializes an MPack writer that writes to a file.
 *
 * @throws mpack_error_memory if allocation fails
 * @throws mpack_error_io if the file cannot be opened
 */
void mpack_writer_init_filename(mpack_writer_t* writer, const char* filename);

/**
 * Deprecated.
 *
 * \deprecated Renamed to mpack_writer_init_filename().
 */
MPACK_INLINE void mpack_writer_init_file(mpack_writer_t* writer, const char* filename) {
    mpack_writer_init_filename(writer, filename);
}

/**
 * Initializes an MPack writer that writes to a libc FILE. This can be used to
 * write to stdout or stderr, or to a file opened separately.
 *
 * @param writer The MPack writer.
 * @param stdfile The FILE.
 * @param close_when_done If true, fclose() will be called on the FILE when it
 *         is no longer needed. If false, the file will not be flushed or
 *         closed when writing is done.
 *
 * @note The writer is buffered. If you want to write other data to the FILE in
 *         between messages, you must flush it first.
 *
 * @see mpack_writer_flush_message
 */
void mpack_writer_init_stdfile(mpack_writer_t* writer, FILE* stdfile, bool close_when_done);
#endif

/** @cond */

#define mpack_writer_init_stack_line_ex(line, writer) \
    char mpack_buf_##line[MPACK_STACK_SIZE]; \
    mpack_writer_init(writer, mpack_buf_##line, sizeof(mpack_buf_##line))

#define mpack_writer_init_stack_line(line, writer) \
    mpack_writer_init_stack_line_ex(line, writer)

/*
 * Initializes an MPack writer using stack space as a buffer. A flush function
 * should be added to the writer to flush the buffer.
 *
 * This is currently undocumented since it's not entirely useful on its own.
 */

#define mpack_writer_init_stack(writer) \
    mpack_writer_init_stack_line(__LINE__, (writer))

/** @endcond */

/**
 * Cleans up the MPack writer, flushing and closing the underlying stream,
 * if any. Returns the final error state of the writer.
 *
 * No flushing is performed if the writer is in an error state. The attached
 * teardown function is called whether or not the writer is in an error state.
 *
 * This will assert in tracking mode if the writer is not in an error
 * state and has any unclosed compound types. If you want to cancel
 * writing in the middle of a document, you need to flag an error on
 * the writer before destroying it (such as mpack_error_data).
 *
 * Note that a writer may raise an error and call your error handler during
 * the final flush. It is safe to longjmp or throw out of this error handler,
 * but if you do, the writer will not be destroyed, and the teardown function
 * will not be called. You can still get the writer's error state, and you
 * must call @ref mpack_writer_destroy() again. (The second call is guaranteed
 * not to call your error handler again since the writer is already in an error
 * state.)
 *
 * @see mpack_writer_set_error_handler
 * @see mpack_writer_set_flush
 * @see mpack_writer_set_teardown
 * @see mpack_writer_flag_error
 * @see mpack_error_data
 */
mpack_error_t mpack_writer_destroy(mpack_writer_t* writer);

/**
 * @}
 */

/**
 * @name Configuration
 * @{
 */

#if MPACK_COMPATIBILITY
/**
 * Sets the version of the MessagePack spec that will be generated.
 *
 * This can be used to interface with older libraries that do not support
 * the newest MessagePack features (such as the @c str8 type.)
 *
 * @note This requires @ref MPACK_COMPATIBILITY.
 */
MPACK_INLINE void mpack_writer_set_version(mpack_writer_t* writer, mpack_version_t version) {
    writer->version = version;
}
#endif

/**
 * Sets the custom pointer to pass to the writer callbacks, such as flush
 * or teardown.
 *
 * @param writer The MPack writer.
 * @param context User data to pass to the writer callbacks.
 *
 * @see mpack_writer_context()
 */
MPACK_INLINE void mpack_writer_set_context(mpack_writer_t* writer, void* context) {
    writer->context = context;
}

/**
 * Returns the custom context for writer callbacks.
 *
 * @see mpack_writer_set_context
 * @see mpack_writer_set_flush
 */
MPACK_INLINE void* mpack_writer_context(mpack_writer_t* writer) {
    return writer->context;
}

/**
 * Sets the flush function to write out the data when the buffer is full.
 *
 * If no flush function is used, trying to write past the end of the
 * buffer will result in mpack_error_too_big.
 *
 * This should normally be used with mpack_writer_set_context() to register
 * a custom pointer to pass to the flush function.
 *
 * @param writer The MPack writer.
 * @param flush The function to write out data from the buffer.
 *
 * @see mpack_writer_context()
 */
void mpack_writer_set_flush(mpack_writer_t* writer, mpack_writer_flush_t flush);

/**
 * Sets the error function to call when an error is flagged on the writer.
 *
 * This should normally be used with mpack_writer_set_context() to register
 * a custom pointer to pass to the error function.
 *
 * See the definition of mpack_writer_error_t for more information about
 * what you can do from an error callback.
 *
 * @see mpack_writer_error_t
 * @param writer The MPack writer.
 * @param error_fn The function to call when an error is flagged on the writer.
 */
MPACK_INLINE void mpack_writer_set_error_handler(mpack_writer_t* writer, mpack_writer_error_t error_fn) {
    writer->error_fn = error_fn;
}

/**
 * Sets the teardown function to call when the writer is destroyed.
 *
 * This should normally be used with mpack_writer_set_context() to register
 * a custom pointer to pass to the teardown function.
 *
 * @param writer The MPack writer.
 * @param teardown The function to call when the writer is destroyed.
 */
MPACK_INLINE void mpack_writer_set_teardown(mpack_writer_t* writer, mpack_writer_teardown_t teardown) {
    writer->teardown = teardown;
}

/**
 * @}
 */

/**
 * @name Core Writer Functions
 * @{
 */

/**
 * Flushes any buffered data to the underlying stream.
 *
 * If the writer is connected to a socket and you are keeping it open,
 * you will want to call this after writing a message (or set of
 * messages) so that the data is actually sent.
 *
 * It is not necessary to call this if you are not keeping the writer
 * open afterwards. You can just call `mpack_writer_destroy()` and it
 * will flush before cleaning up.
 *
 * This will assert if no flush function is assigned to the writer.
 *
 * If write tracking is enabled, this will break and flag @ref
 * mpack_error_bug if the writer has any open compound types, ensuring
 * that no compound types are still open. This prevents a "missing
 * finish" bug from causing a never-ending message.
 */
void mpack_writer_flush_message(mpack_writer_t* writer);

/**
 * Returns the number of bytes currently stored in the buffer. This
 * may be less than the total number of bytes written if bytes have
 * been flushed to an underlying stream.
 */
MPACK_INLINE size_t mpack_writer_buffer_used(mpack_writer_t* writer) {
    return (size_t)(writer->position - writer->buffer);
}

/**
 * Returns the amount of space left in the buffer. This may be reset
 * after a write if bytes are flushed to an underlying stream.
 */
MPACK_INLINE size_t mpack_writer_buffer_left(mpack_writer_t* writer) {
    return (size_t)(writer->end - writer->position);
}

/**
 * Returns the (current) size of the buffer. This may change after a write if
 * the flush callback changes the buffer.
 */
MPACK_INLINE size_t mpack_writer_buffer_size(mpack_writer_t* writer) {
    return (size_t)(writer->end - writer->buffer);
}

/**
 * Places the writer in the given error state, calling the error callback if one
 * is set.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you write it, or if you want to cancel writing in the middle of a
 * document. (The writer will assert if you try to destroy it without error and
 * with unclosed compound types. In this case you should flag mpack_error_data
 * before destroying it.)
 *
 * If the writer is already in an error state, this call is ignored and no
 * error callback is called.
 *
 * @see mpack_writer_destroy
 * @see mpack_error_data
 */
void mpack_writer_flag_error(mpack_writer_t* writer, mpack_error_t error);

/**
 * Queries the error state of the MPack writer.
 *
 * If a writer is in an error state, you should discard all data since the
 * last time the error flag was checked. The error flag cannot be cleared.
 */
MPACK_INLINE mpack_error_t mpack_writer_error(mpack_writer_t* writer) {
    return writer->error;
}

/**
 * Writes a MessagePack object header (an MPack Tag.)
 *
 * If the value is a map, array, string, binary or extension type, the
 * containing elements or bytes must be written separately and the
 * appropriate finish function must be called (as though one of the
 * mpack_start_*() functions was called.)
 *
 * @see mpack_write_bytes()
 * @see mpack_finish_map()
 * @see mpack_finish_array()
 * @see mpack_finish_str()
 * @see mpack_finish_bin()
 * @see mpack_finish_ext()
 * @see mpack_finish_type()
 */
void mpack_write_tag(mpack_writer_t* writer, mpack_tag_t tag);

/**
 * @}
 */

/**
 * @name Integers
 * @{
 */

/** Writes an 8-bit integer in the most efficient packing available. */
void mpack_write_i8(mpack_writer_t* writer, int8_t value);

/** Writes a 16-bit integer in the most efficient packing available. */
void mpack_write_i16(mpack_writer_t* writer, int16_t value);

/** Writes a 32-bit integer in the most efficient packing available. */
void mpack_write_i32(mpack_writer_t* writer, int32_t value);

/** Writes a 64-bit integer in the most efficient packing available. */
void mpack_write_i64(mpack_writer_t* writer, int64_t value);

/** Writes an integer in the most efficient packing available. */
MPACK_INLINE void mpack_write_int(mpack_writer_t* writer, int64_t value) {
    mpack_write_i64(writer, value);
}

/** Writes an 8-bit unsigned integer in the most efficient packing available. */
void mpack_write_u8(mpack_writer_t* writer, uint8_t value);

/** Writes an 16-bit unsigned integer in the most efficient packing available. */
void mpack_write_u16(mpack_writer_t* writer, uint16_t value);

/** Writes an 32-bit unsigned integer in the most efficient packing available. */
void mpack_write_u32(mpack_writer_t* writer, uint32_t value);

/** Writes an 64-bit unsigned integer in the most efficient packing available. */
void mpack_write_u64(mpack_writer_t* writer, uint64_t value);

/** Writes an unsigned integer in the most efficient packing available. */
MPACK_INLINE void mpack_write_uint(mpack_writer_t* writer, uint64_t value) {
    mpack_write_u64(writer, value);
}

/**
 * @}
 */

/**
 * @name Other Basic Types
 * @{
 */

#if MPACK_FLOAT
/** Writes a float. */
void mpack_write_float(mpack_writer_t* writer, float value);
#else
/** Writes a float from a raw uint32_t. */
void mpack_write_raw_float(mpack_writer_t* writer, uint32_t raw_value);
#endif

#if MPACK_DOUBLE
/** Writes a double. */
void mpack_write_double(mpack_writer_t* writer, double value);
#else
/** Writes a double from a raw uint64_t. */
void mpack_write_raw_double(mpack_writer_t* writer, uint64_t raw_value);
#endif

/** Writes a boolean. */
void mpack_write_bool(mpack_writer_t* writer, bool value);

/** Writes a boolean with value true. */
void mpack_write_true(mpack_writer_t* writer);

/** Writes a boolean with value false. */
void mpack_write_false(mpack_writer_t* writer);

/** Writes a nil. */
void mpack_write_nil(mpack_writer_t* writer);

/** Write a pre-encoded messagepack object */
void mpack_write_object_bytes(mpack_writer_t* writer, const char* data, size_t bytes);

#if MPACK_EXTENSIONS
/**
 * Writes a timestamp.
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 *
 * @param writer The writer
 * @param seconds The (signed) number of seconds since 1970-01-01T00:00:00Z.
 * @param nanoseconds The additional number of nanoseconds from 0 to 999,999,999 inclusive.
 */
void mpack_write_timestamp(mpack_writer_t* writer, int64_t seconds, uint32_t nanoseconds);

/**
 * Writes a timestamp with the given number of seconds (and zero nanoseconds).
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 *
 * @param writer The writer
 * @param seconds The (signed) number of seconds since 1970-01-01T00:00:00Z.
 */
MPACK_INLINE void mpack_write_timestamp_seconds(mpack_writer_t* writer, int64_t seconds) {
    mpack_write_timestamp(writer, seconds, 0);
}

/**
 * Writes a timestamp.
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 */
MPACK_INLINE void mpack_write_timestamp_struct(mpack_writer_t* writer, mpack_timestamp_t timestamp) {
    mpack_write_timestamp(writer, timestamp.seconds, timestamp.nanoseconds);
}
#endif

/**
 * @}
 */

/**
 * @name Map and Array Functions
 * @{
 */

/**
 * Opens an array.
 *
 * `count` elements must follow, and mpack_finish_array() must be called
 * when done.
 *
 * If you do not know the number of elements to be written ahead of time, call
 * mpack_build_array() instead.
 *
 * @see mpack_finish_array()
 * @see mpack_build_array() to count the number of elements automatically
 */
void mpack_start_array(mpack_writer_t* writer, uint32_t count);

/**
 * Opens a map.
 *
 * `count * 2` elements must follow, and mpack_finish_map() must be called
 * when done.
 *
 * If you do not know the number of elements to be written ahead of time, call
 * mpack_build_map() instead.
 *
 * Remember that while map elements in MessagePack are implicitly ordered,
 * they are not ordered in JSON. If you need elements to be read back
 * in the order they are written, consider use an array instead.
 *
 * @see mpack_finish_map()
 * @see mpack_build_map() to count the number of key/value pairs automatically
 */
void mpack_start_map(mpack_writer_t* writer, uint32_t count);

MPACK_INLINE void mpack_builder_compound_push(mpack_writer_t* writer) {
    MPACK_UNUSED(writer);

    #if MPACK_BUILDER
    mpack_build_t* build = writer->builder.current_build;
    if (build != NULL) {
        ++build->nested_compound_elements;
    }
    #endif
}

MPACK_INLINE void mpack_builder_compound_pop(mpack_writer_t* writer) {
    MPACK_UNUSED(writer);

    #if MPACK_BUILDER
    mpack_build_t* build = writer->builder.current_build;
    if (build != NULL) {
        mpack_assert(build->nested_compound_elements > 0);
        --build->nested_compound_elements;
    }
    #endif
}

/**
 * Finishes writing an array.
 *
 * This should be called only after a corresponding call to mpack_start_array()
 * and after the array contents are written.
 *
 * In debug mode (or if MPACK_WRITE_TRACKING is not 0), this will track writes
 * to ensure that the correct number of elements are written.
 *
 * @see mpack_start_array()
 */
MPACK_INLINE void mpack_finish_array(mpack_writer_t* writer) {
    mpack_writer_track_pop(writer, mpack_type_array);
    mpack_builder_compound_pop(writer);
}

/**
 * Finishes writing a map.
 *
 * This should be called only after a corresponding call to mpack_start_map()
 * and after the map contents are written.
 *
 * In debug mode (or if MPACK_WRITE_TRACKING is not 0), this will track writes
 * to ensure that the correct number of elements are written.
 *
 * @see mpack_start_map()
 */
MPACK_INLINE void mpack_finish_map(mpack_writer_t* writer) {
    mpack_writer_track_pop(writer, mpack_type_map);
    mpack_builder_compound_pop(writer);
}

/**
 * Starts building an array.
 *
 * Elements must follow, and mpack_complete_array() must be called when done. The
 * number of elements is determined automatically.
 *
 * If you know ahead of time the number of elements in the array, it is more
 * efficient to call mpack_start_array() instead, even if you are already
 * within another open build.
 *
 * Builder containers can be nested within normal (known size) containers and
 * vice versa. You can call mpack_build_array(), then mpack_start_array()
 * inside it, then mpack_build_array() inside that, and so forth.
 *
 * @see mpack_complete_array() to complete this array
 * @see mpack_start_array() if you already know the size of the array
 * @see mpack_build_map() for implementation details
 */
void mpack_build_array(struct mpack_writer_t* writer);

/**
 * Starts building a map.
 *
 * An even number of elements must follow, and mpack_complete_map() must be
 * called when done. The number of elements is determined automatically.
 *
 * If you know ahead of time the number of elements in the map, it is more
 * efficient to call mpack_start_map() instead, even if you are already within
 * another open build.
 *
 * Builder containers can be nested within normal (known size) containers and
 * vice versa. You can call mpack_build_map(), then mpack_start_map() inside
 * it, then mpack_build_map() inside that, and so forth.
 *
 * A writer in build mode diverts writes to a builder buffer that allocates as
 * needed. Once the last map or array being built is completed, the deferred
 * message is composed with computed array and map sizes into the writer.
 * Builder maps and arrays are encoded exactly the same as ordinary maps and
 * arrays in the final message.
 *
 * This indirect encoding is costly, as it incurs at least an extra copy of all
 * data written within a builder (but not additional copies for nested
 * builders.) Expect a speed penalty of half or more.
 *
 * A good strategy is to use this during early development when your messages
 * are constantly changing, and then closer to release when your message
 * formats have stabilized, replace all your build calls with start calls with
 * pre-computed sizes. Or don't, if you find the builder has little impact on
 * performance, because even with builders MPack is extremely fast.
 *
 * @note When an array or map starts being built, nothing will be flushed
 *       until it is completed. If you are building a large message that
 *       does not fit in the output stream, you won't get an error about it
 *       until everything is written.
 *
 * @see mpack_complete_map() to complete this map
 * @see mpack_start_map() if you already know the size of the map
 */
void mpack_build_map(struct mpack_writer_t* writer);

/**
 * Completes an array being built.
 *
 * @see mpack_build_array()
 */
void mpack_complete_array(struct mpack_writer_t* writer);

/**
 * Completes a map being built.
 *
 * @see mpack_build_map()
 */
void mpack_complete_map(struct mpack_writer_t* writer);

/**
 * @}
 */

/**
 * @name Data Helpers
 * @{
 */

/**
 * Writes a string.
 *
 * To stream a string in chunks, use mpack_start_str() instead.
 *
 * MPack does not care about the underlying encoding, but UTF-8 is highly
 * recommended, especially for compatibility with JSON. You should consider
 * calling mpack_write_utf8() instead, especially if you will be reading
 * it back as UTF-8.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 */
void mpack_write_str(mpack_writer_t* writer, const char* str, uint32_t length);

/**
 * Writes a string, ensuring that it is valid UTF-8.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 *
 * @throws mpack_error_invalid if the string is not valid UTF-8
 */
void mpack_write_utf8(mpack_writer_t* writer, const char* str, uint32_t length);

/**
 * Writes a null-terminated string. (The null-terminator is not written.)
 *
 * MPack does not care about the underlying encoding, but UTF-8 is highly
 * recommended, especially for compatibility with JSON. You should consider
 * calling mpack_write_utf8_cstr() instead, especially if you will be reading
 * it back as UTF-8.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 */
void mpack_write_cstr(mpack_writer_t* writer, const char* cstr);

/**
 * Writes a null-terminated string, or a nil node if the given cstr pointer
 * is NULL. (The null-terminator is not written.)
 *
 * MPack does not care about the underlying encoding, but UTF-8 is highly
 * recommended, especially for compatibility with JSON. You should consider
 * calling mpack_write_utf8_cstr_or_nil() instead, especially if you will
 * be reading it back as UTF-8.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 */
void mpack_write_cstr_or_nil(mpack_writer_t* writer, const char* cstr);

/**
 * Writes a null-terminated string, ensuring that it is valid UTF-8. (The
 * null-terminator is not written.)
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 *
 * @throws mpack_error_invalid if the string is not valid UTF-8
 */
void mpack_write_utf8_cstr(mpack_writer_t* writer, const char* cstr);

/**
 * Writes a null-terminated string ensuring that it is valid UTF-8, or
 * writes nil if the given cstr pointer is NULL. (The null-terminator
 * is not written.)
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * You should not call mpack_finish_str() after calling this; this
 * performs both start and finish.
 *
 * @throws mpack_error_invalid if the string is not valid UTF-8
 */
void mpack_write_utf8_cstr_or_nil(mpack_writer_t* writer, const char* cstr);

/**
 * Writes a binary blob.
 *
 * To stream a binary blob in chunks, use mpack_start_bin() instead.
 *
 * You should not call mpack_finish_bin() after calling this; this
 * performs both start and finish.
 */
void mpack_write_bin(mpack_writer_t* writer, const char* data, uint32_t count);

#if MPACK_EXTENSIONS
/**
 * Writes an extension type.
 *
 * To stream an extension blob in chunks, use mpack_start_ext() instead.
 *
 * Extension types [0, 127] are available for application-specific types. Extension
 * types [-128, -1] are reserved for future extensions of MessagePack.
 *
 * You should not call mpack_finish_ext() after calling this; this
 * performs both start and finish.
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 */
void mpack_write_ext(mpack_writer_t* writer, int8_t exttype, const char* data, uint32_t count);
#endif

/**
 * @}
 */

/**
 * @name Chunked Data Functions
 * @{
 */

/**
 * Opens a string. `count` bytes should be written with calls to
 * mpack_write_bytes(), and mpack_finish_str() should be called
 * when done.
 *
 * To write an entire string at once, use mpack_write_str() or
 * mpack_write_cstr() instead.
 *
 * MPack does not care about the underlying encoding, but UTF-8 is highly
 * recommended, especially for compatibility with JSON.
 */
void mpack_start_str(mpack_writer_t* writer, uint32_t count);

/**
 * Opens a binary blob. `count` bytes should be written with calls to
 * mpack_write_bytes(), and mpack_finish_bin() should be called
 * when done.
 */
void mpack_start_bin(mpack_writer_t* writer, uint32_t count);

#if MPACK_EXTENSIONS
/**
 * Opens an extension type. `count` bytes should be written with calls
 * to mpack_write_bytes(), and mpack_finish_ext() should be called
 * when done.
 *
 * Extension types [0, 127] are available for application-specific types. Extension
 * types [-128, -1] are reserved for future extensions of MessagePack.
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 */
void mpack_start_ext(mpack_writer_t* writer, int8_t exttype, uint32_t count);
#endif

/**
 * Writes a portion of bytes for a string, binary blob or extension type which
 * was opened by mpack_write_tag() or one of the mpack_start_*() functions.
 *
 * This can be called multiple times to write the data in chunks, as long as
 * the total amount of bytes written matches the count given when the compound
 * type was started.
 *
 * The corresponding mpack_finish_*() function must be called when done.
 *
 * To write an entire string, binary blob or extension type at
 * once, use one of the mpack_write_*() functions instead.
 *
 * @see mpack_write_tag()
 * @see mpack_start_str()
 * @see mpack_start_bin()
 * @see mpack_start_ext()
 * @see mpack_finish_str()
 * @see mpack_finish_bin()
 * @see mpack_finish_ext()
 * @see mpack_finish_type()
 */
void mpack_write_bytes(mpack_writer_t* writer, const char* data, size_t count);

/**
 * Finishes writing a string.
 *
 * This should be called only after a corresponding call to mpack_start_str()
 * and after the string bytes are written with mpack_write_bytes().
 *
 * This will track writes to ensure that the correct number of elements are written.
 *
 * @see mpack_start_str()
 * @see mpack_write_bytes()
 */
MPACK_INLINE void mpack_finish_str(mpack_writer_t* writer) {
    mpack_writer_track_pop(writer, mpack_type_str);
}

/**
 * Finishes writing a binary blob.
 *
 * This should be called only after a corresponding call to mpack_start_bin()
 * and after the binary bytes are written with mpack_write_bytes().
 *
 * This will track writes to ensure that the correct number of bytes are written.
 *
 * @see mpack_start_bin()
 * @see mpack_write_bytes()
 */
MPACK_INLINE void mpack_finish_bin(mpack_writer_t* writer) {
    mpack_writer_track_pop(writer, mpack_type_bin);
}

#if MPACK_EXTENSIONS
/**
 * Finishes writing an extended type binary data blob.
 *
 * This should be called only after a corresponding call to mpack_start_bin()
 * and after the binary bytes are written with mpack_write_bytes().
 *
 * This will track writes to ensure that the correct number of bytes are written.
 *
 * @note This requires @ref MPACK_EXTENSIONS.
 *
 * @see mpack_start_ext()
 * @see mpack_write_bytes()
 */
MPACK_INLINE void mpack_finish_ext(mpack_writer_t* writer) {
    mpack_writer_track_pop(writer, mpack_type_ext);
}
#endif

/**
 * Finishes writing the given compound type.
 *
 * This will track writes to ensure that the correct number of elements
 * or bytes are written.
 *
 * This can be called with the appropriate type instead the corresponding
 * mpack_finish_*() function if you want to finish a dynamic type.
 */
MPACK_INLINE void mpack_finish_type(mpack_writer_t* writer, mpack_type_t type) {
    mpack_writer_track_pop(writer, type);
}

/**
 * @}
 */

#if MPACK_HAS_GENERIC && !defined(__cplusplus)

/**
 * @name Type-Generic Writers
 * @{
 */

/**
 * @def mpack_write(writer, value)
 *
 * Type-generic writer for primitive types.
 *
 * The compiler will dispatch to an appropriate write function based
 * on the type of the @a value parameter.
 *
 * @note This requires C11 `_Generic` support. (A set of inline overloads
 * are used in C++ to provide the same functionality.)
 *
 * @warning In C11, the indentifiers `true`, `false` and `NULL` are
 * all of type `int`, not `bool` or `void*`! They will emit unexpected
 * types when passed uncast, so be careful when using them.
 */
#if MPACK_FLOAT
    #define MPACK_WRITE_GENERIC_FLOAT float: mpack_write_float,
#else
    #define MPACK_WRITE_GENERIC_FLOAT /*nothing*/
#endif
#if MPACK_DOUBLE
    #define MPACK_WRITE_GENERIC_DOUBLE double: mpack_write_double,
#else
    #define MPACK_WRITE_GENERIC_DOUBLE /*nothing*/
#endif
#define mpack_write(writer, value) \
    _Generic(((void)0, value),                      \
              int8_t: mpack_write_i8,               \
             int16_t: mpack_write_i16,              \
             int32_t: mpack_write_i32,              \
             int64_t: mpack_write_i64,              \
             uint8_t: mpack_write_u8,               \
            uint16_t: mpack_write_u16,              \
            uint32_t: mpack_write_u32,              \
            uint64_t: mpack_write_u64,              \
                bool: mpack_write_bool,             \
            MPACK_WRITE_GENERIC_FLOAT               \
            MPACK_WRITE_GENERIC_DOUBLE              \
              char *: mpack_write_cstr_or_nil,      \
        const char *: mpack_write_cstr_or_nil       \
    )(writer, value)

/**
 * @def mpack_write_kv(writer, key, value)
 *
 * Type-generic writer for key-value pairs of null-terminated string
 * keys and primitive values.
 *
 * @warning @a writer may be evaluated multiple times.
 *
 * @warning In C11, the indentifiers `true`, `false` and `NULL` are
 * all of type `int`, not `bool` or `void*`! They will emit unexpected
 * types when passed uncast, so be careful when using them.
 *
 * @param writer The writer.
 * @param key A null-terminated C string.
 * @param value A primitive type supported by mpack_write().
 */
#define mpack_write_kv(writer, key, value) do {     \
    mpack_write_cstr(writer, key);                  \
    mpack_write(writer, value);                     \
} while (0)

/**
 * @}
 */

#endif // MPACK_HAS_GENERIC && !defined(__cplusplus)

// The rest of this file contains C++ overloads, so we end extern "C" here.
MPACK_EXTERN_C_END

#if defined(__cplusplus) || defined(MPACK_DOXYGEN)

/**
 * @name C++ write overloads
 * @{
 */

/*
 * C++ generic writers for primitive values
 */

#ifdef MPACK_DOXYGEN
#undef mpack_write
#undef mpack_write_kv
#endif

MPACK_INLINE void mpack_write(mpack_writer_t* writer, int8_t value) {
    mpack_write_i8(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, int16_t value) {
    mpack_write_i16(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, int32_t value) {
    mpack_write_i32(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, int64_t value) {
    mpack_write_i64(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, uint8_t value) {
    mpack_write_u8(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, uint16_t value) {
    mpack_write_u16(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, uint32_t value) {
    mpack_write_u32(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, uint64_t value) {
    mpack_write_u64(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, bool value) {
    mpack_write_bool(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, float value) {
    mpack_write_float(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, double value) {
    mpack_write_double(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, char *value) {
    mpack_write_cstr_or_nil(writer, value);
}

MPACK_INLINE void mpack_write(mpack_writer_t* writer, const char *value) {
    mpack_write_cstr_or_nil(writer, value);
}

/* C++ generic write for key-value pairs */

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, int8_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_i8(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, int16_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_i16(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, int32_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_i32(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, int64_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_i64(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, uint8_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_u8(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, uint16_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_u16(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, uint32_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_u32(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, uint64_t value) {
    mpack_write_cstr(writer, key);
    mpack_write_u64(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, bool value) {
    mpack_write_cstr(writer, key);
    mpack_write_bool(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, float value) {
    mpack_write_cstr(writer, key);
    mpack_write_float(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, double value) {
    mpack_write_cstr(writer, key);
    mpack_write_double(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, char *value) {
    mpack_write_cstr(writer, key);
    mpack_write_cstr_or_nil(writer, value);
}

MPACK_INLINE void mpack_write_kv(mpack_writer_t* writer, const char *key, const char *value) {
    mpack_write_cstr(writer, key);
    mpack_write_cstr_or_nil(writer, value);
}

/**
 * @}
 */

#endif /* __cplusplus */

/**
 * @}
 */

MPACK_SILENCE_WARNINGS_END

#endif // MPACK_WRITER

#endif
