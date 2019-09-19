/*
 * Copyright (c) 2015-2018 Nicholas Fraser
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

#include "mpack-common.h"

#if MPACK_DEBUG && MPACK_STDIO
#include <stdarg.h>
#endif

const char* mpack_error_to_string(mpack_error_t error) {
    #if MPACK_STRINGS
    switch (error) {
        #define MPACK_ERROR_STRING_CASE(e) case e: return #e
        MPACK_ERROR_STRING_CASE(mpack_ok);
        MPACK_ERROR_STRING_CASE(mpack_error_io);
        MPACK_ERROR_STRING_CASE(mpack_error_invalid);
        MPACK_ERROR_STRING_CASE(mpack_error_unsupported);
        MPACK_ERROR_STRING_CASE(mpack_error_type);
        MPACK_ERROR_STRING_CASE(mpack_error_too_big);
        MPACK_ERROR_STRING_CASE(mpack_error_memory);
        MPACK_ERROR_STRING_CASE(mpack_error_bug);
        MPACK_ERROR_STRING_CASE(mpack_error_data);
        MPACK_ERROR_STRING_CASE(mpack_error_eof);
        #undef MPACK_ERROR_STRING_CASE
    }
    mpack_assert(0, "unrecognized error %i", (int)error);
    return "(unknown mpack_error_t)";
    #else
    MPACK_UNUSED(error);
    return "";
    #endif
}

const char* mpack_type_to_string(mpack_type_t type) {
    #if MPACK_STRINGS
    switch (type) {
        #define MPACK_TYPE_STRING_CASE(e) case e: return #e
        MPACK_TYPE_STRING_CASE(mpack_type_missing);
        MPACK_TYPE_STRING_CASE(mpack_type_nil);
        MPACK_TYPE_STRING_CASE(mpack_type_bool);
        MPACK_TYPE_STRING_CASE(mpack_type_float);
        MPACK_TYPE_STRING_CASE(mpack_type_double);
        MPACK_TYPE_STRING_CASE(mpack_type_int);
        MPACK_TYPE_STRING_CASE(mpack_type_uint);
        MPACK_TYPE_STRING_CASE(mpack_type_str);
        MPACK_TYPE_STRING_CASE(mpack_type_bin);
        MPACK_TYPE_STRING_CASE(mpack_type_array);
        MPACK_TYPE_STRING_CASE(mpack_type_map);
        #if MPACK_EXTENSIONS
        MPACK_TYPE_STRING_CASE(mpack_type_ext);
        #endif
        #undef MPACK_TYPE_STRING_CASE
    }
    mpack_assert(0, "unrecognized type %i", (int)type);
    return "(unknown mpack_type_t)";
    #else
    MPACK_UNUSED(type);
    return "";
    #endif
}

int mpack_tag_cmp(mpack_tag_t left, mpack_tag_t right) {

    // positive numbers may be stored as int; convert to uint
    if (left.type == mpack_type_int && left.v.i >= 0) {
        left.type = mpack_type_uint;
        left.v.u = (uint64_t)left.v.i;
    }
    if (right.type == mpack_type_int && right.v.i >= 0) {
        right.type = mpack_type_uint;
        right.v.u = (uint64_t)right.v.i;
    }

    if (left.type != right.type)
        return ((int)left.type < (int)right.type) ? -1 : 1;

    switch (left.type) {
        case mpack_type_missing: // fallthrough
        case mpack_type_nil:
            return 0;

        case mpack_type_bool:
            return (int)left.v.b - (int)right.v.b;

        case mpack_type_int:
            if (left.v.i == right.v.i)
                return 0;
            return (left.v.i < right.v.i) ? -1 : 1;

        case mpack_type_uint:
            if (left.v.u == right.v.u)
                return 0;
            return (left.v.u < right.v.u) ? -1 : 1;

        case mpack_type_array:
        case mpack_type_map:
            if (left.v.n == right.v.n)
                return 0;
            return (left.v.n < right.v.n) ? -1 : 1;

        case mpack_type_str:
        case mpack_type_bin:
            if (left.v.l == right.v.l)
                return 0;
            return (left.v.l < right.v.l) ? -1 : 1;

        #if MPACK_EXTENSIONS
        case mpack_type_ext:
            if (left.exttype == right.exttype) {
                if (left.v.l == right.v.l)
                    return 0;
                return (left.v.l < right.v.l) ? -1 : 1;
            }
            return (int)left.exttype - (int)right.exttype;
        #endif

        // floats should not normally be compared for equality. we compare
        // with memcmp() to silence compiler warnings, but this will return
        // equal if both are NaNs with the same representation (though we may
        // want this, for instance if you are for some bizarre reason using
        // floats as map keys.) i'm not sure what the right thing to
        // do is here. check for NaN first? always return false if the type
        // is float? use operator== and pragmas to silence compiler warning?
        // please send me your suggestions.
        // note also that we don't convert floats to doubles, so when this is
        // used for ordering purposes, all floats are ordered before all
        // doubles.
        case mpack_type_float:
            return mpack_memcmp(&left.v.f, &right.v.f, sizeof(left.v.f));
        case mpack_type_double:
            return mpack_memcmp(&left.v.d, &right.v.d, sizeof(left.v.d));
    }

    mpack_assert(0, "unrecognized type %i", (int)left.type);
    return false;
}

#if MPACK_DEBUG && MPACK_STDIO
static char mpack_hex_char(uint8_t hex_value) {
    // Older compilers (e.g. GCC 4.4.7) promote the result of this ternary to
    // int and warn under -Wconversion, so we have to cast it back to char.
    return (char)((hex_value < 10) ? (char)('0' + hex_value) : (char)('a' + (hex_value - 10)));
}

static void mpack_tag_debug_complete_bin_ext(mpack_tag_t tag, size_t string_length, char* buffer, size_t buffer_size,
        const char* prefix, size_t prefix_size)
{
    // If at any point in this function we run out of space in the buffer, we
    // bail out. The outer tag print wrapper will make sure we have a
    // null-terminator.

    if (string_length == 0 || string_length >= buffer_size)
        return;
    buffer += string_length;
    buffer_size -= string_length;

    size_t total = mpack_tag_bytes(&tag);
    if (total == 0) {
        strncpy(buffer, ">", buffer_size);
        return;
    }

    strncpy(buffer, ": ", buffer_size);
    if (buffer_size < 2)
        return;
    buffer += 2;
    buffer_size -= 2;

    size_t hex_bytes = 0;
    for (size_t i = 0; i < MPACK_PRINT_BYTE_COUNT && i < prefix_size && buffer_size > 2; ++i) {
        uint8_t byte = (uint8_t)prefix[i];
        buffer[0] = mpack_hex_char((uint8_t)(byte >> 4));
        buffer[1] = mpack_hex_char((uint8_t)(byte & 0xfu));
        buffer += 2;
        buffer_size -= 2;
        ++hex_bytes;
    }

    if (buffer_size != 0)
        mpack_snprintf(buffer, buffer_size, "%s>", (total > hex_bytes) ? "..." : "");
}

static void mpack_tag_debug_pseudo_json_bin(mpack_tag_t tag, char* buffer, size_t buffer_size,
        const char* prefix, size_t prefix_size)
{
    mpack_assert(mpack_tag_type(&tag) == mpack_type_bin);
    size_t length = (size_t)mpack_snprintf(buffer, buffer_size, "<binary data of length %u", tag.v.l);
    mpack_tag_debug_complete_bin_ext(tag, length, buffer, buffer_size, prefix, prefix_size);
}

#if MPACK_EXTENSIONS
static void mpack_tag_debug_pseudo_json_ext(mpack_tag_t tag, char* buffer, size_t buffer_size,
        const char* prefix, size_t prefix_size)
{
    mpack_assert(mpack_tag_type(&tag) == mpack_type_ext);
    size_t length = (size_t)mpack_snprintf(buffer, buffer_size, "<ext data of type %i and length %u",
            mpack_tag_ext_exttype(&tag), mpack_tag_ext_length(&tag));
    mpack_tag_debug_complete_bin_ext(tag, length, buffer, buffer_size, prefix, prefix_size);
}
#endif

static void mpack_tag_debug_pseudo_json_impl(mpack_tag_t tag, char* buffer, size_t buffer_size,
        const char* prefix, size_t prefix_size)
{
    switch (tag.type) {
        case mpack_type_missing:
            mpack_snprintf(buffer, buffer_size, "<missing!>");
            return;
        case mpack_type_nil:
            mpack_snprintf(buffer, buffer_size, "null");
            return;
        case mpack_type_bool:
            mpack_snprintf(buffer, buffer_size, tag.v.b ? "true" : "false");
            return;
        case mpack_type_int:
            mpack_snprintf(buffer, buffer_size, "%" PRIi64, tag.v.i);
            return;
        case mpack_type_uint:
            mpack_snprintf(buffer, buffer_size, "%" PRIu64, tag.v.u);
            return;
        case mpack_type_float:
            mpack_snprintf(buffer, buffer_size, "%f", tag.v.f);
            return;
        case mpack_type_double:
            mpack_snprintf(buffer, buffer_size, "%f", tag.v.d);
            return;

        case mpack_type_str:
            mpack_snprintf(buffer, buffer_size, "<string of %u bytes>", tag.v.l);
            return;
        case mpack_type_bin:
            mpack_tag_debug_pseudo_json_bin(tag, buffer, buffer_size, prefix, prefix_size);
            return;
        #if MPACK_EXTENSIONS
        case mpack_type_ext:
            mpack_tag_debug_pseudo_json_ext(tag, buffer, buffer_size, prefix, prefix_size);
            return;
        #endif

        case mpack_type_array:
            mpack_snprintf(buffer, buffer_size, "<array of %u elements>", tag.v.n);
            return;
        case mpack_type_map:
            mpack_snprintf(buffer, buffer_size, "<map of %u key-value pairs>", tag.v.n);
            return;
    }

    mpack_snprintf(buffer, buffer_size, "<unknown!>");
}

void mpack_tag_debug_pseudo_json(mpack_tag_t tag, char* buffer, size_t buffer_size,
        const char* prefix, size_t prefix_size)
{
    mpack_assert(buffer_size > 0, "buffer size cannot be zero!");
    buffer[0] = 0;

    mpack_tag_debug_pseudo_json_impl(tag, buffer, buffer_size, prefix, prefix_size);

    // We always null-terminate the buffer manually just in case the snprintf()
    // function doesn't null-terminate when the string doesn't fit.
    buffer[buffer_size - 1] = 0;
}

static void mpack_tag_debug_describe_impl(mpack_tag_t tag, char* buffer, size_t buffer_size) {
    switch (tag.type) {
        case mpack_type_missing:
            mpack_snprintf(buffer, buffer_size, "missing");
            return;
        case mpack_type_nil:
            mpack_snprintf(buffer, buffer_size, "nil");
            return;
        case mpack_type_bool:
            mpack_snprintf(buffer, buffer_size, tag.v.b ? "true" : "false");
            return;
        case mpack_type_int:
            mpack_snprintf(buffer, buffer_size, "int %" PRIi64, tag.v.i);
            return;
        case mpack_type_uint:
            mpack_snprintf(buffer, buffer_size, "uint %" PRIu64, tag.v.u);
            return;
        case mpack_type_float:
            mpack_snprintf(buffer, buffer_size, "float %f", tag.v.f);
            return;
        case mpack_type_double:
            mpack_snprintf(buffer, buffer_size, "double %f", tag.v.d);
            return;
        case mpack_type_str:
            mpack_snprintf(buffer, buffer_size, "str of %u bytes", tag.v.l);
            return;
        case mpack_type_bin:
            mpack_snprintf(buffer, buffer_size, "bin of %u bytes", tag.v.l);
            return;
        #if MPACK_EXTENSIONS
        case mpack_type_ext:
            mpack_snprintf(buffer, buffer_size, "ext of type %i, %u bytes",
                    mpack_tag_ext_exttype(&tag), mpack_tag_ext_length(&tag));
            return;
        #endif
        case mpack_type_array:
            mpack_snprintf(buffer, buffer_size, "array of %u elements", tag.v.n);
            return;
        case mpack_type_map:
            mpack_snprintf(buffer, buffer_size, "map of %u key-value pairs", tag.v.n);
            return;
    }

    mpack_snprintf(buffer, buffer_size, "unknown!");
}

void mpack_tag_debug_describe(mpack_tag_t tag, char* buffer, size_t buffer_size) {
    mpack_assert(buffer_size > 0, "buffer size cannot be zero!");
    buffer[0] = 0;

    mpack_tag_debug_describe_impl(tag, buffer, buffer_size);

    // We always null-terminate the buffer manually just in case the snprintf()
    // function doesn't null-terminate when the string doesn't fit.
    buffer[buffer_size - 1] = 0;
}
#endif



#if MPACK_READ_TRACKING || MPACK_WRITE_TRACKING

#ifndef MPACK_TRACKING_INITIAL_CAPACITY
// seems like a reasonable number. we grow by doubling, and it only
// needs to be as long as the maximum depth of the message.
#define MPACK_TRACKING_INITIAL_CAPACITY 8
#endif

mpack_error_t mpack_track_init(mpack_track_t* track) {
    track->count = 0;
    track->capacity = MPACK_TRACKING_INITIAL_CAPACITY;
    track->elements = (mpack_track_element_t*)MPACK_MALLOC(sizeof(mpack_track_element_t) * track->capacity);
    if (track->elements == NULL)
        return mpack_error_memory;
    return mpack_ok;
}

mpack_error_t mpack_track_grow(mpack_track_t* track) {
    mpack_assert(track->elements, "null track elements!");
    mpack_assert(track->count == track->capacity, "incorrect growing?");

    size_t new_capacity = track->capacity * 2;

    mpack_track_element_t* new_elements = (mpack_track_element_t*)mpack_realloc(track->elements,
            sizeof(mpack_track_element_t) * track->count, sizeof(mpack_track_element_t) * new_capacity);
    if (new_elements == NULL)
        return mpack_error_memory;

    track->elements = new_elements;
    track->capacity = new_capacity;
    return mpack_ok;
}

mpack_error_t mpack_track_push(mpack_track_t* track, mpack_type_t type, uint32_t count) {
    mpack_assert(track->elements, "null track elements!");
    mpack_log("track pushing %s count %i\n", mpack_type_to_string(type), (int)count);

    // grow if needed
    if (track->count == track->capacity) {
        mpack_error_t error = mpack_track_grow(track);
        if (error != mpack_ok)
            return error;
    }

    // insert new track
    track->elements[track->count].type = type;
    track->elements[track->count].left = count;
    track->elements[track->count].key_needs_value = false;
    ++track->count;
    return mpack_ok;
}

mpack_error_t mpack_track_pop(mpack_track_t* track, mpack_type_t type) {
    mpack_assert(track->elements, "null track elements!");
    mpack_log("track popping %s\n", mpack_type_to_string(type));

    if (track->count == 0) {
        mpack_break("attempting to close a %s but nothing was opened!", mpack_type_to_string(type));
        return mpack_error_bug;
    }

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type != type) {
        mpack_break("attempting to close a %s but the open element is a %s!",
                mpack_type_to_string(type), mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->key_needs_value) {
        mpack_assert(type == mpack_type_map, "key_needs_value can only be true for maps!");
        mpack_break("attempting to close a %s but an odd number of elements were written",
                mpack_type_to_string(type));
        return mpack_error_bug;
    }

    if (element->left != 0) {
        mpack_break("attempting to close a %s but there are %i %s left",
                mpack_type_to_string(type), element->left,
                (type == mpack_type_map || type == mpack_type_array) ? "elements" : "bytes");
        return mpack_error_bug;
    }

    --track->count;
    return mpack_ok;
}

mpack_error_t mpack_track_peek_element(mpack_track_t* track, bool read) {
    MPACK_UNUSED(read);
    mpack_assert(track->elements, "null track elements!");

    // if there are no open elements, that's fine, we can read/write elements at will
    if (track->count == 0)
        return mpack_ok;

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type != mpack_type_map && element->type != mpack_type_array) {
        mpack_break("elements cannot be %s within an %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left == 0 && !element->key_needs_value) {
        mpack_break("too many elements %s for %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    return mpack_ok;
}

mpack_error_t mpack_track_element(mpack_track_t* track, bool read) {
    mpack_error_t error = mpack_track_peek_element(track, read);
    if (track->count == 0 || error != mpack_ok)
        return error;

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type == mpack_type_map) {
        if (!element->key_needs_value) {
            element->key_needs_value = true;
            return mpack_ok; // don't decrement
        }
        element->key_needs_value = false;
    }

    --element->left;
    return mpack_ok;
}

mpack_error_t mpack_track_bytes(mpack_track_t* track, bool read, size_t count) {
    MPACK_UNUSED(read);
    mpack_assert(track->elements, "null track elements!");

    if (count > UINT32_MAX) {
        mpack_break("%s more bytes than could possibly fit in a str/bin/ext!",
                read ? "reading" : "writing");
        return mpack_error_bug;
    }

    if (track->count == 0) {
        mpack_break("bytes cannot be %s with no open bin, str or ext", read ? "read" : "written");
        return mpack_error_bug;
    }

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type == mpack_type_map || element->type == mpack_type_array) {
        mpack_break("bytes cannot be %s within an %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left < count) {
        mpack_break("too many bytes %s for %s", read ? "read" : "written",
                mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    element->left -= (uint32_t)count;
    return mpack_ok;
}

mpack_error_t mpack_track_str_bytes_all(mpack_track_t* track, bool read, size_t count) {
    mpack_error_t error = mpack_track_bytes(track, read, count);
    if (error != mpack_ok)
        return error;

    mpack_track_element_t* element = &track->elements[track->count - 1];

    if (element->type != mpack_type_str) {
        mpack_break("the open type must be a string, not a %s", mpack_type_to_string(element->type));
        return mpack_error_bug;
    }

    if (element->left != 0) {
        mpack_break("not all bytes were read; the wrong byte count was requested for a string read.");
        return mpack_error_bug;
    }

    return mpack_ok;
}

mpack_error_t mpack_track_check_empty(mpack_track_t* track) {
    if (track->count != 0) {
        mpack_break("unclosed %s", mpack_type_to_string(track->elements[0].type));
        return mpack_error_bug;
    }
    return mpack_ok;
}

mpack_error_t mpack_track_destroy(mpack_track_t* track, bool cancel) {
    mpack_error_t error = cancel ? mpack_ok : mpack_track_check_empty(track);
    if (track->elements) {
        MPACK_FREE(track->elements);
        track->elements = NULL;
    }
    return error;
}
#endif



static bool mpack_utf8_check_impl(const uint8_t* str, size_t count, bool allow_null) {
    while (count > 0) {
        uint8_t lead = str[0];

        // NUL
        if (!allow_null && lead == '\0') // we don't allow NUL bytes in MPack C-strings
            return false;

        // ASCII
        if (lead <= 0x7F) {
            ++str;
            --count;

        // 2-byte sequence
        } else if ((lead & 0xE0) == 0xC0) {
            if (count < 2) // truncated sequence
                return false;

            uint8_t cont = str[1];
            if ((cont & 0xC0) != 0x80) // not a continuation byte
                return false;

            str += 2;
            count -= 2;

            uint32_t z = ((uint32_t)(lead & ~0xE0) << 6) |
                          (uint32_t)(cont & ~0xC0);

            if (z < 0x80) // overlong sequence
                return false;

        // 3-byte sequence
        } else if ((lead & 0xF0) == 0xE0) {
            if (count < 3) // truncated sequence
                return false;

            uint8_t cont1 = str[1];
            if ((cont1 & 0xC0) != 0x80) // not a continuation byte
                return false;
            uint8_t cont2 = str[2];
            if ((cont2 & 0xC0) != 0x80) // not a continuation byte
                return false;

            str += 3;
            count -= 3;

            uint32_t z = ((uint32_t)(lead  & ~0xF0) << 12) |
                         ((uint32_t)(cont1 & ~0xC0) <<  6) |
                          (uint32_t)(cont2 & ~0xC0);

            if (z < 0x800) // overlong sequence
                return false;
            if (z >= 0xD800 && z <= 0xDFFF) // surrogate
                return false;

        // 4-byte sequence
        } else if ((lead & 0xF8) == 0xF0) {
            if (count < 4) // truncated sequence
                return false;

            uint8_t cont1 = str[1];
            if ((cont1 & 0xC0) != 0x80) // not a continuation byte
                return false;
            uint8_t cont2 = str[2];
            if ((cont2 & 0xC0) != 0x80) // not a continuation byte
                return false;
            uint8_t cont3 = str[3];
            if ((cont3 & 0xC0) != 0x80) // not a continuation byte
                return false;

            str += 4;
            count -= 4;

            uint32_t z = ((uint32_t)(lead  & ~0xF8) << 18) |
                         ((uint32_t)(cont1 & ~0xC0) << 12) |
                         ((uint32_t)(cont2 & ~0xC0) <<  6) |
                          (uint32_t)(cont3 & ~0xC0);

            if (z < 0x10000) // overlong sequence
                return false;
            if (z > 0x10FFFF) // codepoint limit
                return false;

        } else {
            return false; // continuation byte without a lead, or lead for a 5-byte sequence or longer
        }
    }
    return true;
}

bool mpack_utf8_check(const char* str, size_t bytes) {
    return mpack_utf8_check_impl((const uint8_t*)str, bytes, true);
}

bool mpack_utf8_check_no_null(const char* str, size_t bytes) {
    return mpack_utf8_check_impl((const uint8_t*)str, bytes, false);
}

bool mpack_str_check_no_null(const char* str, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i)
        if (str[i] == '\0')
            return false;
    return true;
}

#if MPACK_DEBUG && MPACK_STDIO
void mpack_print_append(mpack_print_t* print, const char* data, size_t count) {

    // copy whatever fits into the buffer
    size_t copy = print->size - print->count;
    if (copy > count)
        copy = count;
    mpack_memcpy(print->buffer + print->count, data, copy);
    print->count += copy;
    data += copy;
    count -= copy;

    // if we don't need to flush or can't flush there's nothing else to do
    if (count == 0 || print->callback == NULL)
        return;

    // flush the buffer
    print->callback(print->context, print->buffer, print->count);

    if (count > print->size / 2) {
        // flush the rest of the data
        print->count = 0;
        print->callback(print->context, data, count);
    } else {
        // copy the rest of the data into the buffer
        mpack_memcpy(print->buffer, data, count);
        print->count = count;
    }

}

void mpack_print_flush(mpack_print_t* print) {
    if (print->count > 0 && print->callback != NULL) {
        print->callback(print->context, print->buffer, print->count);
        print->count = 0;
    }
}

void mpack_print_file_callback(void* context, const char* data, size_t count) {
    FILE* file = (FILE*)context;
    fwrite(data, 1, count, file);
}
#endif
