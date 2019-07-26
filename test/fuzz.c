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

#ifdef MPACK_FUZZ

/*
 * fuzz.c is a test program to assist with fuzzing MPack. It:
 *
 * - decodes stdin with the dynamic Reader API;
 * - encodes the data to a growable buffer with the Write API;
 * - parses the resulting buffer with the Node API;
 * - and finally, prints a debug dump of the node tree to stdout.
 *
 * It thus passes all data through three major components of MPack (but not
 * the Expect API.)
 */

#include "mpack/mpack.h"

#ifndef MPACK_FUZZ_CONFIG_H
#error "This should be built with fuzz-config.h as a prefix header."
#endif

static void print_callback(void* context, const char* data, size_t count) {
    fwrite(data, 1, count, stdout);
}

static void transfer_bytes(mpack_reader_t* reader, mpack_writer_t* writer, uint32_t count) {
    if (mpack_should_read_bytes_inplace(reader, count)) {
        const char* data = mpack_read_bytes_inplace(reader, count);
        if (mpack_reader_error(reader) == mpack_ok)
            mpack_write_bytes(writer, data, count);
        return;
    }

    while (count > 0) {
        char buffer[79];
        uint32_t step = (count < sizeof(buffer)) ? count : sizeof(buffer);
        mpack_read_bytes(reader, buffer, step);
        if (mpack_reader_error(reader) != mpack_ok)
            return;
        mpack_write_bytes(writer, buffer, step);
        count -= step;
    }
}

static void transfer_element(mpack_reader_t* reader, mpack_writer_t* writer, int depth) {

    // We apply a depth limit manually right now to avoid a stack overflow. A
    // depth limit should probably be added to the reader and tree at some
    // point because even though the reader and tree can themselves handle
    // arbitrary depths, any dynamic use that doesn't account for this is
    // likely to be vulnerable to such stack overflows.
    if (depth >= 1024) {
        fprintf(stderr, "hit depth limit!\n");
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return;
    }
    ++depth;

    mpack_tag_t tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok) {
        fprintf(stderr, "error reading tag!\n");
        return;
    }

    /*
    static char describe_buffer[64];
    mpack_tag_debug_describe(tag, describe_buffer, sizeof(describe_buffer));
    printf("%s\n", describe_buffer);
    */

    mpack_write_tag(writer, tag);

    switch (tag.type) {
        #if MPACK_EXTENSIONS
        case mpack_type_ext: // fallthrough
        #endif
        case mpack_type_str: // fallthrough
        case mpack_type_bin:
            transfer_bytes(reader, writer, mpack_tag_bytes(&tag));
            if (mpack_reader_error(reader) != mpack_ok)
                return;
            mpack_done_type(reader, tag.type);
            mpack_finish_type(writer, tag.type);
            break;

        case mpack_type_map:
            for (uint32_t i = 0; i < mpack_tag_map_count(&tag); ++i) {
                transfer_element(reader, writer, depth);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
                transfer_element(reader, writer, depth);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
            }
            mpack_done_map(reader);
            mpack_finish_map(writer);
            break;

        case mpack_type_array:
            for (uint32_t i = 0; i < mpack_tag_array_count(&tag); ++i) {
                transfer_element(reader, writer, depth);
                if (mpack_reader_error(reader) != mpack_ok)
                    return;
            }
            mpack_done_array(reader);
            mpack_finish_array(writer);
            break;

        default:
            break;
    }
}

int main(int argc, char** argv) {
    char* data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_reader_t reader;
    mpack_reader_init_stdfile(&reader, stdin, false);

    transfer_element(&reader, &writer, 0);

    if (mpack_reader_destroy(&reader) != mpack_ok || mpack_writer_destroy(&writer) != mpack_ok) {
        fprintf(stderr, "error in reader or writer!\n");
        return EXIT_FAILURE;
    }

    mpack_tree_t tree;
    mpack_tree_init_stdfile(&tree, stdin, 0, false);
    mpack_tree_parse(&tree);
    if (mpack_tree_error(&tree) != mpack_ok) {
        fprintf(stderr, "error parsing tree!\n");
        return EXIT_FAILURE;
    }

    mpack_node_print_to_callback(mpack_tree_root(&tree), print_callback, NULL);

    if (mpack_tree_destroy(&tree) != mpack_ok) {
        fprintf(stderr, "error printing or destroying tree!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#else
typedef int mpack_pedantic_allow_empty_translation_unit;
#endif
