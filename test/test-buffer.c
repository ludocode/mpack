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

#include "test-buffer.h"

#include "test-expect.h"
#include "test-write.h"

static const char test_numbers[] =

        "\x02" // 2
        "\x11" // 17
        "\x1d" // 29
        "\x2b" // 43
        "\x3b" // 59
        "\x47" // 71
        "\x59" // 89
        "\x65" // 101

        "\xcc\x83" // 131
        "\xcc\x95" // 149
        "\xcc\x9d" // 157
        "\xcc\xad" // 173
        "\xcc\xbf" // 191
        "\xcc\xc7" // 199
        "\xcc\xdf" // 223
        "\xcc\xe3" // 227

        "\xcd\x01\x01" // 257
        "\xcd\x1d\x5d" // 7517
        "\xcd\x39\xaf" // 14767
        "\xcd\x56\x0b" // 22027
        "\xcd\x72\x55" // 29269
        "\xcd\x8e\xab" // 36523
        "\xcd\xab\x01" // 43777
        "\xcd\xc7\x57" // 51031

        "\xce\x00\x01\x00\x01" // 65537
        "\xce\x1c\x72\xaa\xb3" // 477276851
        "\xce\x38\xe4\x55\x59" // 954488153
        "\xce\x55\x56\x00\x19" // 1431699481
        "\xce\x71\xc7\xaa\xab" // 1908910763
        "\xce\x8e\x39\x55\x77" // 2386122103
        "\xce\xaa\xab\x00\x17" // 2863333399
        "\xce\xc7\x1c\xaa\xa9" // 3340544681

        "\xcf\x00\x00\x00\x01\x00\x00\x00\x0f" // 4294967311
        "\xcf\x1a\xf2\x86\xbd\x86\xbc\xa2\x67" // 1941762537917555303
        "\xcf\x35\xe5\x0d\x7a\x0d\x79\x44\x0f" // 3883525071540143119
        "\xcf\x50\xd7\x94\x36\x94\x35\xe4\x51" // 5825287605162730577
        "\xcf\x6b\xca\x1a\xf3\x1a\xf2\x88\x31" // 7767050138785318961
        "\xcf\x86\xbc\xa1\xaf\xa1\xaf\x28\x3f" // 9708812672407906367
        "\xcf\xa1\xaf\x28\x6c\x28\x6b\xc8\x11" // 11650575206030493713
        "\xcf\xbc\xa1\xaf\x28\xaf\x28\x68\x03" // 13592337739653081091

        ;

static const char test_strings[] =
    "\x9F"
        "\xA0"
        "\xA1""a"
        "\xA2""ab"
        "\xA3""abc"
        "\xA4""abcd"
        "\xA5""abcde"
        "\xA6""abcdef"
        "\xA7""abcdefg"
        "\xA8""abcdefgh"
        "\xA9""abcdefghi"
        "\xAA""abcdefghij"
        "\xAB""abcdefghijk"
        "\xAC""abcdefghijkl"
        "\xAD""abcdefghijklm"
        "\xAE""abcdefghijklmn";


// a semi-random list of buffer sizes we will test with. each buffer
// test is run with each of these buffer sizes to test the fill and
// flush functions.
static const size_t test_buffer_sizes[] = {
    32, 33, 34, 35, 36, 37, 39, 43, 48, 51,
    52, 53, 57, 59, 64, 67, 89, 127, 128,
    129, 131, 160, 163, 191, 192, 193,
    251, 256, 257, 509, 512, 521,
    1021, 1024, 1031, 2039, 2048, 2053,
    #ifndef __AVR__
    4093, 4096, 4099, 7919, 8192,
    6384, 32768,
    #endif
};

#if MPACK_READER
typedef struct test_fill_state_t {
    const char* data;
    size_t remaining;
} test_fill_state_t;

static size_t test_buffer_fill(mpack_reader_t* reader, char* buffer, size_t count) {
    test_fill_state_t* state = (test_fill_state_t*)reader->context;
    if (state->remaining < count)
        count = state->remaining;
    memcpy(buffer, state->data, count);
    state->data += count;
    state->remaining -= count;
    return count;
}
#endif

#if MPACK_WRITER
typedef struct test_flush_state_t {
    char* data;
    size_t remaining;
} test_flush_state_t;

static void test_buffer_flush(mpack_writer_t* writer, const char* buffer, size_t count) {
    test_flush_state_t* state = (test_flush_state_t*)writer->context;
    if (state->remaining < count) {
        mpack_writer_flag_error(writer, mpack_error_too_big);
        return;
    }
    memcpy(state->data, buffer, count);
    state->data += count;
    state->remaining -= count;
}
#endif

#if MPACK_EXPECT
static void test_expect_buffer_values(mpack_reader_t* reader) {
    TEST_READ_NOERROR(reader, 2 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 17 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 29 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 43 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 59 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 71 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 89 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 101 == mpack_expect_u8(reader));

    TEST_READ_NOERROR(reader, 131 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 149 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 157 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 173 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 191 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 199 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 223 == mpack_expect_u8(reader));
    TEST_READ_NOERROR(reader, 227 == mpack_expect_u8(reader));

    TEST_READ_NOERROR(reader, 257 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 7517 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 14767 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 22027 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 29269 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 36523 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 43777 == mpack_expect_u16(reader));
    TEST_READ_NOERROR(reader, 51031 == mpack_expect_u16(reader));

    TEST_READ_NOERROR(reader, 65537 == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, 477276851 == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, 954488153 == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, 1431699481 == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, 1908910763 == mpack_expect_u32(reader));

    // when using UINT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(2386122103) == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(2863333399) == mpack_expect_u32(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(3340544681) == mpack_expect_u32(reader));

    TEST_READ_NOERROR(reader, MPACK_UINT64_C(4294967311) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(1941762537917555303) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(3883525071540143119) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(5825287605162730577) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(7767050138785318961) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(9708812672407906367) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(11650575206030493713) == mpack_expect_u64(reader));
    TEST_READ_NOERROR(reader, MPACK_UINT64_C(13592337739653081091) == mpack_expect_u64(reader));
}
#endif

#if MPACK_WRITER
static void test_write_buffer_values(mpack_writer_t* writer) {
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 2));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 17));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 29));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 43));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 59));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 71));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 89));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 101));

    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 131));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 149));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 157));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 173));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 191));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 199));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 223));
    TEST_WRITE_NOERROR(writer, mpack_write_u8(writer, 227));

    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 257));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 7517));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 14767));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 22027));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 29269));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 36523));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 43777));
    TEST_WRITE_NOERROR(writer, mpack_write_u16(writer, 51031));

    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, 65537));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, 477276851));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, 954488153));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, 1431699481));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, 1908910763));

    // when using UINT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, MPACK_UINT64_C(2386122103)));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, MPACK_UINT64_C(2863333399)));
    TEST_WRITE_NOERROR(writer, mpack_write_u32(writer, MPACK_UINT64_C(3340544681)));

    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(4294967311)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(1941762537917555303)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(3883525071540143119)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(5825287605162730577)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(7767050138785318961)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(9708812672407906367)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(11650575206030493713)));
    TEST_WRITE_NOERROR(writer, mpack_write_u64(writer, MPACK_UINT64_C(13592337739653081091)));
}
#endif

#if MPACK_EXPECT
static void test_expect_buffer(void) {
    size_t i;
    for (i = 0; i < sizeof(test_buffer_sizes) / sizeof(test_buffer_sizes[0]); ++i) {

        // initialize the reader with our buffer reader function
        mpack_reader_t reader;
        size_t size = test_buffer_sizes[i];
        char* buffer = (char*)malloc(size);
        test_fill_state_t state = {test_numbers, sizeof(test_numbers) - 1};
        mpack_reader_init(&reader, buffer, size, 0);
        mpack_reader_set_fill(&reader, test_buffer_fill);
        mpack_reader_set_context(&reader, &state);

        // read and destroy, ensuring no errors
        test_expect_buffer_values(&reader);
        TEST_READER_DESTROY_NOERROR(&reader);
        free(buffer);

    }
}
#endif

#if MPACK_WRITER
static void test_write_buffer(void) {
    size_t i;
    for (i = 0; i < sizeof(test_buffer_sizes) / sizeof(test_buffer_sizes[0]); ++i) {
        size_t size = test_buffer_sizes[i];
        size_t output_size = 
            #ifdef __AVR__
            0xfff
            #else
            0xfffff
            #endif
            ;
        char* output = (char*)malloc(output_size);

        // initialize the writer with our buffer writer function
        mpack_writer_t writer;
        char* buffer = (char*)malloc(size);
        char* pos = output;
        test_flush_state_t state = {output, output_size};
        mpack_writer_init(&writer, buffer, size);
        mpack_writer_set_flush(&writer, test_buffer_flush);
        mpack_writer_set_context(&writer, &state);

        // read and destroy, ensuring no errors
        test_write_buffer_values(&writer);
        TEST_WRITER_DESTROY_NOERROR(&writer);
        free(buffer);

        // check output
        TEST_TRUE(output_size - state.remaining == sizeof(test_numbers) - 1,
                "output contains %i bytes but %i were expected",
                (int)(pos - output), (int)sizeof(test_numbers) - 1);
        TEST_TRUE(memcmp(output, test_numbers, sizeof(test_numbers) - 1) == 0,
                "output does not match test buffer");
        free(output);
    }
}
#endif

#if MPACK_READER
static void test_inplace_buffer(void) {
    size_t i;
    for (i = 0; i < sizeof(test_buffer_sizes) / sizeof(test_buffer_sizes[0]); ++i) {

        // initialize the reader with our buffer reader function
        mpack_reader_t reader;
        size_t size = test_buffer_sizes[i];
        char* buffer = (char*)malloc(size);
        test_fill_state_t state = {test_strings, sizeof(test_strings) - 1};
        mpack_reader_init(&reader, buffer, size, 0);
        mpack_reader_set_fill(&reader, test_buffer_fill);
        mpack_reader_set_context(&reader, &state);

        // read the array
        mpack_tag_t tag = mpack_read_tag(&reader);
        TEST_TRUE(tag.type == mpack_type_array, "wrong type: %i %s", (int)tag.type, mpack_type_to_string(tag.type));
        TEST_TRUE(tag.v.n == 15, "wrong array count: %i", tag.v.n);

        // check each string, using inplace if it's less than or equal to the
        // length of the buffer size
        static const char* ref = "abcdefghijklmn";
        const char* val;
        char r[15];
        size_t j;
        for (j = 0; j < 15; ++j) {
            mpack_tag_t peek = mpack_peek_tag(&reader);
            tag = mpack_read_tag(&reader);
            TEST_TRUE(mpack_tag_equal(peek, tag), "peeked tag does not match read tag");
            TEST_TRUE(tag.type == mpack_type_str, "wrong type: %i %s", (int)tag.type, mpack_type_to_string(tag.type));
            TEST_TRUE(tag.v.l == j, "string is the wrong length: %i bytes", (int)tag.v.l);
            if (tag.v.l <= reader.size) {
                val = mpack_read_bytes_inplace(&reader, tag.v.l);
            } else {
                mpack_read_bytes(&reader, r, tag.v.l);
                val = r;
            }
            TEST_TRUE(memcmp(val, ref, tag.v.l) == 0, "strings do not match!");
            mpack_done_str(&reader);
        }

        // destroy, ensuring no errors
        mpack_done_array(&reader);
        TEST_READER_DESTROY_NOERROR(&reader);
        free(buffer);

    }
}
#endif

void test_buffers(void) {
    MPACK_UNUSED(test_numbers);
    MPACK_UNUSED(test_strings);
    MPACK_UNUSED(test_buffer_sizes);

    #if MPACK_EXPECT
    test_expect_buffer();
    #endif
    #if MPACK_WRITER
    test_write_buffer();
    #endif
    #if MPACK_READER
    test_inplace_buffer();
    #endif
}

