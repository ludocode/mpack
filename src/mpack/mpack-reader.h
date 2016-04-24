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

/**
 * @file
 *
 * Declares the core MPack Tag Reader.
 */

#ifndef MPACK_READER_H
#define MPACK_READER_H 1

#include "mpack-common.h"

MPACK_HEADER_START

#if MPACK_READER

#if MPACK_READ_TRACKING
struct mpack_track_t;
#endif

// The denominator to determine whether a read is a small
// fraction of the buffer size.
#define MPACK_READER_SMALL_FRACTION_DENOMINATOR 32

/**
 * @defgroup reader Core Reader API
 *
 * The MPack Core Reader API contains functions for imperatively reading
 * dynamically typed data from a MessagePack stream. This forms the basis
 * of the Expect API.
 *
 * @{
 */

/**
 * @def MPACK_READER_MINIMUM_BUFFER_SIZE
 *
 * The minimum buffer size for a reader with a fill function.
 */
#define MPACK_READER_MINIMUM_BUFFER_SIZE 32

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
 * The MPack reader's fill function. It should fill the buffer with at
 * least one byte and at most the given @c count, returning the number
 * of bytes written to the buffer.
 *
 * In case of error, it should flag an appropriate error on the reader
 * (usually @ref mpack_error_io), or simply return zero. If zero is
 * returned, mpack_error_io is raised.
 *
 * @note When reading from a stream, you should only copy and return
 * the bytes that are immediately available. It is always safe to return
 * less than the requested count as long as some non-zero number of bytes
 * are read; if more bytes are needed, the read function will simply be
 * called again.
 */
typedef size_t (*mpack_reader_fill_t)(mpack_reader_t* reader, char* buffer, size_t count);

/**
 * The MPack reader's skip function. It should discard the given number
 * of bytes from the source (for example by seeking forward.)
 *
 * In case of error, it should flag an appropriate error on the reader.
 */
typedef void (*mpack_reader_skip_t)(mpack_reader_t* reader, size_t count);

/**
 * An error handler function to be called when an error is flagged on
 * the reader.
 *
 * The error handler will only be called once on the first error flagged;
 * any subsequent reads and errors are ignored, and the reader is
 * permanently in that error state.
 *
 * MPack is safe against non-local jumps out of error handler callbacks.
 * This means you are allowed to longjmp or throw an exception (in C++,
 * Objective-C, or with SEH) out of this callback.
 *
 * Bear in mind when using longjmp that local non-volatile variables that
 * have changed are undefined when setjmp() returns, so you can't put the
 * reader on the stack in the same activation frame as the setjmp without
 * declaring it volatile.
 *
 * You must still eventually destroy the reader. It is not destroyed
 * automatically when an error is flagged. It is safe to destroy the
 * reader within this error callback, but you will either need to perform
 * a non-local jump, or store something in your context to identify
 * that the reader is destroyed since any future accesses to it cause
 * undefined behavior.
 */
typedef void (*mpack_reader_error_t)(mpack_reader_t* reader, mpack_error_t error);

/**
 * A teardown function to be called when the reader is destroyed.
 */
typedef void (*mpack_reader_teardown_t)(mpack_reader_t* reader);

/* Hide internals from documentation */
/** @cond */

struct mpack_reader_t {
    void* context;                    /* Context for reader callbacks */
    mpack_reader_fill_t fill;         /* Function to read bytes into the buffer */
    mpack_reader_error_t error_fn;    /* Function to call on error */
    mpack_reader_teardown_t teardown; /* Function to teardown the context on destroy */
    mpack_reader_skip_t skip;         /* Function to skip bytes from the source */

    char* buffer;       /* Writeable byte buffer */
    size_t size;        /* Size of the buffer */

    const char* data;   /* Available data */
    size_t left;        /* How many bytes are left in the available data */

    mpack_error_t error;  /* Error state */

    #if MPACK_READ_TRACKING
    mpack_track_t track; /* Stack of map/array/str/bin/ext reads */
    #endif
};

/** @endcond */

/**
 * @name Lifecycle Functions
 * @{
 */

/**
 * Initializes an MPack reader with the given buffer. The reader does
 * not assume ownership of the buffer, but the buffer must be writeable
 * if a fill function will be used to refill it.
 *
 * @param reader The MPack reader.
 * @param buffer The buffer with which to read MessagePack data.
 * @param size The size of the buffer.
 * @param count The number of bytes already in the buffer.
 */
void mpack_reader_init(mpack_reader_t* reader, char* buffer, size_t size, size_t count);

/**
 * Initializes an MPack reader directly into an error state. Use this if you
 * are writing a wrapper to mpack_reader_init() which can fail its setup.
 */
void mpack_reader_init_error(mpack_reader_t* reader, mpack_error_t error);

/**
 * Initializes an MPack reader to parse a pre-loaded contiguous chunk of data. The
 * reader does not assume ownership of the data.
 *
 * @param reader The MPack reader.
 * @param data The data to parse.
 * @param count The number of bytes pointed to by data.
 */
void mpack_reader_init_data(mpack_reader_t* reader, const char* data, size_t count);

#if MPACK_STDIO
/**
 * Initializes an MPack reader that reads from a file.
 */
void mpack_reader_init_file(mpack_reader_t* reader, const char* filename);
#endif

/**
 * @def mpack_reader_init_stack(reader)
 * @hideinitializer
 *
 * Initializes an MPack reader using stack space as a buffer. A fill function
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
 * Cleans up the MPack reader, ensuring that all compound elements
 * have been completely read. Returns the final error state of the
 * reader.
 *
 * This will assert in tracking mode if the reader is not in an error
 * state and has any incomplete reads. If you want to cancel reading
 * in the middle of a document, you need to flag an error on the reader
 * before destroying it (such as mpack_error_data).
 *
 * @see mpack_read_tag()
 * @see mpack_reader_flag_error()
 * @see mpack_error_data
 */
mpack_error_t mpack_reader_destroy(mpack_reader_t* reader);

/**
 * @}
 */

/**
 * @name Callbacks
 * @{
 */

/**
 * Sets the custom pointer to pass to the reader callbacks, such as fill
 * or teardown.
 *
 * @param reader The MPack reader.
 * @param context User data to pass to the reader callbacks.
 */
MPACK_INLINE void mpack_reader_set_context(mpack_reader_t* reader, void* context) {
    reader->context = context;
}

/**
 * Sets the fill function to refill the data buffer when it runs out of data.
 *
 * If no fill function is used, truncated MessagePack data results in
 * mpack_error_invalid (since the buffer is assumed to contain a
 * complete MessagePack object.)
 *
 * If a fill function is used, truncated MessagePack data usually
 * results in mpack_error_io (since the fill function fails to get
 * the missing data.)
 *
 * This should normally be used with mpack_reader_set_context() to register
 * a custom pointer to pass to the fill function.
 *
 * @param reader The MPack reader.
 * @param fill The function to fetch additional data into the buffer.
 */
void mpack_reader_set_fill(mpack_reader_t* reader, mpack_reader_fill_t fill);

/**
 * Sets the skip function to discard bytes from the source stream.
 *
 * It's not necessary to implement this function. If the stream is not
 * seekable, don't set a skip callback. The reader will fall back to
 * using the fill function instead.
 *
 * This should normally be used with mpack_reader_set_context() to register
 * a custom pointer to pass to the skip function.
 *
 * The skip function is ignored in size-optimized builds to reduce code
 * size. Data will be skipped with the fill function when necessary.
 *
 * @param reader The MPack reader.
 * @param skip The function to discard bytes from the source stream.
 */
void mpack_reader_set_skip(mpack_reader_t* reader, mpack_reader_skip_t skip);

/**
 * Sets the error function to call when an error is flagged on the reader.
 *
 * This should normally be used with mpack_reader_set_context() to register
 * a custom pointer to pass to the error function.
 *
 * See the definition of mpack_reader_error_t for more information about
 * what you can do from an error callback.
 *
 * @see mpack_reader_error_t
 * @param reader The MPack reader.
 * @param error_fn The function to call when an error is flagged on the reader.
 */
MPACK_INLINE void mpack_reader_set_error_handler(mpack_reader_t* reader, mpack_reader_error_t error_fn) {
    reader->error_fn = error_fn;
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
MPACK_INLINE void mpack_reader_set_teardown(mpack_reader_t* reader, mpack_reader_teardown_t teardown) {
    reader->teardown = teardown;
}

/**
 * @}
 */

/**
 * @name Core Reader Functions
 * @{
 */

/**
 * Queries the error state of the MPack reader.
 *
 * If a reader is in an error state, you should discard all data since the
 * last time the error flag was checked. The error flag cannot be cleared.
 */
MPACK_INLINE mpack_error_t mpack_reader_error(mpack_reader_t* reader) {
    return reader->error;
}

/**
 * Places the reader in the given error state, calling the error callback if one
 * is set.
 *
 * This allows you to externally flag errors, for example if you are validating
 * data as you read it.
 *
 * If the reader is already in an error state, this call is ignored and no
 * error callback is called.
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
MPACK_INLINE mpack_error_t mpack_reader_flag_if_error(mpack_reader_t* reader, mpack_error_t error) {
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
 * If you know the length of the MPack chunk beforehand, it's better to instead
 * have your fill function limit the data it reads so that the reader does not
 * have extra data. In this case you can simply check that this returns zero.
 *
 * Returns 0 if the reader is in an error state.
 *
 * @param reader The MPack reader from which to query remaining data.
 * @param data [out] A pointer to the remaining data, or NULL.
 * @return The number of bytes remaining in the buffer.
 */
size_t mpack_reader_remaining(mpack_reader_t* reader, const char** data);

/**
 * Reads a MessagePack object header (an MPack tag.)
 *
 * If an error occurs, the reader is placed in an error state and a
 * nil tag is returned. If the reader is already in an error state,
 * a nil tag is returned.
 *
 * If the type is compound (i.e. is a map, array, string, binary or
 * extension type), additional reads are required to get the contained
 * data, and the corresponding done function must be called when done.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 * 
 * @see mpack_read_bytes()
 * @see mpack_done_array()
 * @see mpack_done_map()
 * @see mpack_done_str()
 * @see mpack_done_bin()
 * @see mpack_done_ext()
 */
mpack_tag_t mpack_read_tag(mpack_reader_t* reader);

/**
 * Parses the next MessagePack object header (an MPack tag) without
 * advancing the reader.
 *
 * If an error occurs, the reader is placed in an error state and a
 * nil tag is returned. If the reader is already in an error state,
 * a nil tag is returned.
 *
 * @note Maps in JSON are unordered, so it is recommended not to expect
 * a specific ordering for your map values in case your data is converted
 * to/from JSON.
 *
 * @see mpack_read_tag()
 * @see mpack_discard()
 */
mpack_tag_t mpack_peek_tag(mpack_reader_t* reader);

/**
 * @}
 */

/**
 * @name String and Data Functions
 * @{
 */

/**
 * Skips bytes from the underlying stream. This is used only to
 * skip the contents of a string, binary blob or extension object.
 */
void mpack_skip_bytes(mpack_reader_t* reader, size_t count);

/**
 * Reads bytes from a string, binary blob or extension object, copying
 * them into the given buffer.
 *
 * A str, bin or ext must have been opened by a call to mpack_read_tag()
 * which yielded one of these types, or by a call to an expect function
 * such as mpack_expect_str() or mpack_expect_bin().
 *
 * If an error occurs, the buffer contents are undefined.
 *
 * This can be called multiple times for a single str, bin or ext
 * to read the data in chunks. The total data read must add up
 * to the size of the object.
 *
 * @param reader The MPack reader
 * @param p The buffer in which to copy the bytes
 * @param count The number of bytes to read
 */
void mpack_read_bytes(mpack_reader_t* reader, char* p, size_t count);

/**
 * Reads bytes from a string, ensures that the string is valid UTF-8,
 * and copies the bytes into the given buffer.
 *
 * A string must have been opened by a call to mpack_read_tag() which
 * yielded a string, or by a call to an expect function such as
 * mpack_expect_str().
 *
 * The given byte count must match the complete size of the string as
 * returned by the tag or expect function. You must ensure that the
 * buffer fits the data.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * If an error occurs, the buffer contents are undefined.
 *
 * Unlike mpack_read_bytes(), this cannot be used to read the data in
 * chunks (since this might split a character's UTF-8 bytes, and the
 * reader does not keep track of the UTF-8 decoding state between reads.)
 *
 * @throws mpack_error_type if the string contains invalid UTF-8.
 */
void mpack_read_utf8(mpack_reader_t* reader, char* p, size_t byte_count);

/**
 * Reads bytes from a string, ensures that the string contains no NUL
 * bytes, copies the bytes into the given buffer and adds a null-terminator.
 *
 * A string must have been opened by a call to mpack_read_tag() which
 * yielded a string, or by a call to an expect function such as
 * mpack_expect_str().
 *
 * The given byte count must match the size of the string as returned
 * by the tag or expect function. The string will only be copied if
 * the buffer is large enough to store it.
 *
 * If an error occurs, the buffer will contain an empty string.
 *
 * @note If you know the object will be a string before reading it,
 * it is highly recommended to use mpack_expect_cstr() instead.
 * Alternatively you could use mpack_peek_tag() and call
 * mpack_expect_cstr() if it's a string.
 *
 * @throws mpack_error_too_big if the string plus null-terminator is larger than the given buffer size
 * @throws mpack_error_type if the string contains a null byte.
 *
 * @see mpack_peek_tag()
 * @see mpack_expect_cstr()
 * @see mpack_expect_utf8_cstr()
 */
void mpack_read_cstr(mpack_reader_t* reader, char* buf, size_t buffer_size, size_t byte_count);

/**
 * Reads bytes from a string, ensures that the string is valid UTF-8
 * with no NUL bytes, copies the bytes into the given buffer and adds a
 * null-terminator.
 *
 * A string must have been opened by a call to mpack_read_tag() which
 * yielded a string, or by a call to an expect function such as
 * mpack_expect_str().
 *
 * The given byte count must match the size of the string as returned
 * by the tag or expect function. The string will only be copied if
 * the buffer is large enough to store it.
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed, but without the NUL character, since
 * it cannot be represented in a null-terminated string.
 *
 * If an error occurs, the buffer will contain an empty string.
 *
 * @note If you know the object will be a string before reading it,
 * it is highly recommended to use mpack_expect_utf8_cstr() instead.
 * Alternatively you could use mpack_peek_tag() and call
 * mpack_expect_utf8_cstr() if it's a string.
 *
 * @throws mpack_error_too_big if the string plus null-terminator is larger than the given buffer size
 * @throws mpack_error_type if the string contains invalid UTF-8 or a null byte.
 *
 * @see mpack_peek_tag()
 * @see mpack_expect_utf8_cstr()
 */
void mpack_read_utf8_cstr(mpack_reader_t* reader, char* buf, size_t buffer_size, size_t byte_count);

#ifdef MPACK_MALLOC
/** @cond */
// This can optionally add a null-terminator, but it does not check
// whether the data contains null bytes. This must be done separately
// in a cstring read function (possibly as part of a UTF-8 check.)
char* mpack_read_bytes_alloc_impl(mpack_reader_t* reader, size_t count, bool null_terminated);
/** @endcond */

/**
 * Reads bytes from a string, binary blob or extension object, allocating
 * storage for them and returning the allocated pointer.
 *
 * The allocated string must be freed with MPACK_FREE() (or simply free()
 * if MPack's allocator hasn't been customized.)
 *
 * Returns NULL if any error occurs, or if count is zero.
 */
MPACK_INLINE char* mpack_read_bytes_alloc(mpack_reader_t* reader, size_t count) {
    return mpack_read_bytes_alloc_impl(reader, count, false);
}
#endif

/**
 * Reads bytes from a string, binary blob or extension object in-place in
 * the buffer. This can be used to avoid copying the data.
 *
 * A str, bin or ext must have been opened by a call to mpack_read_tag()
 * which yielded one of these types, or by a call to an expect function
 * such as mpack_expect_str() or mpack_expect_bin().
 *
 * If the bytes are from a string, the string is not null-terminated! Use
 * mpack_read_cstr() to copy the string into a buffer and add a null-terminator.
 *
 * The returned pointer is invalidated on the next read, or when the buffer
 * is destroyed.
 *
 * The reader will move data around in the buffer if needed to ensure that
 * the pointer can always be returned, so this should only be used if
 * count is very small compared to the buffer size. If you need to check
 * whether a small size is reasonable (for example you intend to handle small and
 * large sizes differently), you can call mpack_should_read_bytes_inplace().
 *
 * This can be called multiple times for a single str, bin or ext
 * to read the data in chunks. The total data read must add up
 * to the size of the object.
 *
 * NULL is returned if the reader is in an error state.
 *
 * @throws mpack_error_too_big if the requested size is larger than the buffer size
 *
 * @see mpack_should_read_bytes_inplace()
 */
const char* mpack_read_bytes_inplace(mpack_reader_t* reader, size_t count);

/**
 * Reads bytes from a string in-place in the buffer and ensures they are
 * valid UTF-8. This can be used to avoid copying the data.
 *
 * A string must have been opened by a call to mpack_read_tag() which
 * yielded a string, or by a call to an expect function such as
 * mpack_expect_str().
 *
 * The string is not null-terminated! Use mpack_read_utf8_cstr() to
 * copy the string into a buffer and add a null-terminator.
 *
 * The returned pointer is invalidated on the next read, or when the buffer
 * is destroyed.
 *
 * The reader will move data around in the buffer if needed to ensure that
 * the pointer can always be returned, so this should only be used if
 * count is very small compared to the buffer size. If you need to check
 * whether a small size is reasonable (for example you intend to handle small and
 * large sizes differently), you can call mpack_should_read_bytes_inplace().
 *
 * This does not accept any UTF-8 variant such as Modified UTF-8, CESU-8 or
 * WTF-8. Only pure UTF-8 is allowed.
 *
 * Unlike mpack_read_bytes_inplace(), this cannot be used to read the data in
 * chunks (since this might split a character's UTF-8 bytes, and the
 * reader does not keep track of the UTF-8 decoding state between reads.)
 *
 * NULL is returned if the reader is in an error state.
 *
 * @throws mpack_error_type if the string contains invalid UTF-8
 * @throws mpack_error_too_big if the requested size is larger than the buffer size
 *
 * @see mpack_should_read_bytes_inplace()
 */
const char* mpack_read_utf8_inplace(mpack_reader_t* reader, size_t count);

/**
 * Returns true if it's a good idea to read the given number of bytes
 * in-place.
 *
 * If the read will be larger than some small fraction of the buffer size,
 * this will return false to avoid shuffling too much data back and forth
 * in the buffer.
 *
 * Use this if you're expecting arbitrary size data, and you want to read
 * in-place for the best performance when possible but will fall back to
 * a normal read if the data is too large.
 *
 * @see mpack_read_bytes_inplace()
 */
MPACK_INLINE bool mpack_should_read_bytes_inplace(mpack_reader_t* reader, size_t count) {
    return (reader->size == 0 || count <= reader->size / MPACK_READER_SMALL_FRACTION_DENOMINATOR);
}

/**
 * @}
 */

/**
 * @name Core Reader Functions
 * @{
 */

#if MPACK_READ_TRACKING
/**
 * Finishes reading the given type.
 *
 * This will track reads to ensure that the correct number of elements
 * or bytes are read.
 */
void mpack_done_type(mpack_reader_t* reader, mpack_type_t type);
#else
MPACK_INLINE void mpack_done_type(mpack_reader_t* reader, mpack_type_t type) {
    MPACK_UNUSED(reader);
    MPACK_UNUSED(type);
}
#endif

/**
 * Finishes reading an array.
 *
 * This will track reads to ensure that the correct number of elements are read.
 */
MPACK_INLINE void mpack_done_array(mpack_reader_t* reader) {
    mpack_done_type(reader, mpack_type_array);
}

/**
 * @fn mpack_done_map(mpack_reader_t* reader)
 *
 * Finishes reading a map.
 *
 * This will track reads to ensure that the correct number of elements are read.
 */
MPACK_INLINE void mpack_done_map(mpack_reader_t* reader) {
    mpack_done_type(reader, mpack_type_map);
}

/**
 * @fn mpack_done_str(mpack_reader_t* reader)
 *
 * Finishes reading a string.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
MPACK_INLINE void mpack_done_str(mpack_reader_t* reader) {
    mpack_done_type(reader, mpack_type_str);
}

/**
 * @fn mpack_done_bin(mpack_reader_t* reader)
 *
 * Finishes reading a binary data blob.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
MPACK_INLINE void mpack_done_bin(mpack_reader_t* reader) {
    mpack_done_type(reader, mpack_type_bin);
}

/**
 * @fn mpack_done_ext(mpack_reader_t* reader)
 *
 * Finishes reading an extended type binary data blob.
 *
 * This will track reads to ensure that the correct number of bytes are read.
 */
MPACK_INLINE void mpack_done_ext(mpack_reader_t* reader) {
    mpack_done_type(reader, mpack_type_ext);
}

/**
 * Reads and discards the next object. This will read and discard all
 * contained data as well if it is a compound type.
 */
void mpack_discard(mpack_reader_t* reader);

/**
 * @}
 */

#if MPACK_STDIO
/**
 * @name Debugging Functions
 * @{
 */

/**
 * Converts a blob of MessagePack to pseudo-JSON for debugging purposes
 * and pretty-prints it to the given file.
 */
void mpack_print_file(const char* data, size_t len, FILE* file);

#ifndef MPACK_GCOV
/**
 * Converts a blob of MessagePack to pseudo-JSON for debugging purposes
 * and pretty-prints it to stdout.
 */
MPACK_INLINE void mpack_print(const char* data, size_t len) {
    mpack_print_file(data, len, stdout);
}
#endif

/**
 * @}
 */
#endif

/**
 * @}
 */



#if MPACK_INTERNAL

bool mpack_reader_ensure_straddle(mpack_reader_t* reader, size_t count);

/*
 * Ensures there are at least @c count bytes left in the
 * data, raising an error and returning false if more
 * data cannot be made available.
 */
MPACK_INLINE bool mpack_reader_ensure(mpack_reader_t* reader, size_t count) {
    mpack_assert(count != 0, "cannot ensure zero bytes!");
    mpack_assert(reader->error == mpack_ok, "reader cannot be in an error state!");

    if (count <= reader->left)
        return true;
    return mpack_reader_ensure_straddle(reader, count);
}

void mpack_read_native_big(mpack_reader_t* reader, char* p, size_t count);

// Reads count bytes into p, deferring to mpack_read_native_big() if more
// bytes are needed than are available in the buffer.
MPACK_INLINE void mpack_read_native(mpack_reader_t* reader, char* p, size_t count) {
    mpack_assert(count == 0 || p != NULL, "data pointer for %i bytes is NULL", (int)count);

    if (count > reader->left) {
        mpack_read_native_big(reader, p, count);
    } else {
        mpack_memcpy(p, reader->data, count);
        reader->data += count;
        reader->left -= count;
    }
}

#if MPACK_READ_TRACKING
#define MPACK_READER_TRACK(reader, error_expr) \
    (((reader)->error == mpack_ok) ? mpack_reader_flag_if_error((reader), (error_expr)) : (reader)->error)
#else
#define MPACK_READER_TRACK(reader, error_expr) (MPACK_UNUSED(reader), mpack_ok)
#endif

MPACK_INLINE mpack_error_t mpack_reader_track_element(mpack_reader_t* reader) {
    return MPACK_READER_TRACK(reader, mpack_track_element(&reader->track, true));
}

MPACK_INLINE mpack_error_t mpack_reader_track_peek_element(mpack_reader_t* reader) {
    return MPACK_READER_TRACK(reader, mpack_track_peek_element(&reader->track, true));
}

MPACK_INLINE mpack_error_t mpack_reader_track_bytes(mpack_reader_t* reader, uint64_t count) {
    MPACK_UNUSED(count);
    return MPACK_READER_TRACK(reader, mpack_track_bytes(&reader->track, true, count));
}

MPACK_INLINE mpack_error_t mpack_reader_track_str_bytes_all(mpack_reader_t* reader, uint64_t count) {
    MPACK_UNUSED(count);
    return MPACK_READER_TRACK(reader, mpack_track_str_bytes_all(&reader->track, true, count));
}

#endif



#endif

MPACK_HEADER_END

#endif

