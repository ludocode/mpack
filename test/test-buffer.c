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

#include "test-buffer.h"

#include "test-read.h"
#include "test-write.h"

const char test_buffer[] = 

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

// a semi-random list of buffer sizes we will test with. each buffer
// test is run with each of these buffer sizes to test the fill and
// flush functions.
const int test_buffer_sizes[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9,
    11, 13, 16, 17, 19, 23, 29, 32,
    37, 48, 64, 67, 89, 127, 128,
    129, 131, 160, 163, 191, 192, 193,
    251, 256, 257, 509, 512, 521,
    1021, 1024, 1031, 2039, 2048, 2053,
    4093, 4096, 4099, 7919, 8192,
    16384, 32768
};

static void test_read_buffer_values(mpack_reader_t* reader) {
    test_read_noerror(reader, 2 == mpack_expect_u8(reader));
    test_read_noerror(reader, 17 == mpack_expect_u8(reader));
    test_read_noerror(reader, 29 == mpack_expect_u8(reader));
    test_read_noerror(reader, 43 == mpack_expect_u8(reader));
    test_read_noerror(reader, 59 == mpack_expect_u8(reader));
    test_read_noerror(reader, 71 == mpack_expect_u8(reader));
    test_read_noerror(reader, 89 == mpack_expect_u8(reader));
    test_read_noerror(reader, 101 == mpack_expect_u8(reader));

    test_read_noerror(reader, 131 == mpack_expect_u8(reader));
    test_read_noerror(reader, 149 == mpack_expect_u8(reader));
    test_read_noerror(reader, 157 == mpack_expect_u8(reader));
    test_read_noerror(reader, 173 == mpack_expect_u8(reader));
    test_read_noerror(reader, 191 == mpack_expect_u8(reader));
    test_read_noerror(reader, 199 == mpack_expect_u8(reader));
    test_read_noerror(reader, 223 == mpack_expect_u8(reader));
    test_read_noerror(reader, 227 == mpack_expect_u8(reader));

    test_read_noerror(reader, 257 == mpack_expect_u16(reader));
    test_read_noerror(reader, 7517 == mpack_expect_u16(reader));
    test_read_noerror(reader, 14767 == mpack_expect_u16(reader));
    test_read_noerror(reader, 22027 == mpack_expect_u16(reader));
    test_read_noerror(reader, 29269 == mpack_expect_u16(reader));
    test_read_noerror(reader, 36523 == mpack_expect_u16(reader));
    test_read_noerror(reader, 43777 == mpack_expect_u16(reader));
    test_read_noerror(reader, 51031 == mpack_expect_u16(reader));

    test_read_noerror(reader, 65537 == mpack_expect_u32(reader));
    test_read_noerror(reader, 477276851 == mpack_expect_u32(reader));
    test_read_noerror(reader, 954488153 == mpack_expect_u32(reader));
    test_read_noerror(reader, 1431699481 == mpack_expect_u32(reader));
    test_read_noerror(reader, 1908910763 == mpack_expect_u32(reader));

    // when using UINT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    test_read_noerror(reader, UINT64_C(2386122103) == mpack_expect_u32(reader));
    test_read_noerror(reader, UINT64_C(2863333399) == mpack_expect_u32(reader));
    test_read_noerror(reader, UINT64_C(3340544681) == mpack_expect_u32(reader));


    test_read_noerror(reader, UINT64_C(4294967311) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(1941762537917555303) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(3883525071540143119) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(5825287605162730577) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(7767050138785318961) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(9708812672407906367) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(11650575206030493713) == mpack_expect_u64(reader));
    test_read_noerror(reader, UINT64_C(13592337739653081091) == mpack_expect_u64(reader));
}

static void test_write_buffer_values(mpack_writer_t* writer) {
    test_write_noerror(writer, mpack_write_u8(writer, 2));
    test_write_noerror(writer, mpack_write_u8(writer, 17));
    test_write_noerror(writer, mpack_write_u8(writer, 29));
    test_write_noerror(writer, mpack_write_u8(writer, 43));
    test_write_noerror(writer, mpack_write_u8(writer, 59));
    test_write_noerror(writer, mpack_write_u8(writer, 71));
    test_write_noerror(writer, mpack_write_u8(writer, 89));
    test_write_noerror(writer, mpack_write_u8(writer, 101));

    test_write_noerror(writer, mpack_write_u8(writer, 131));
    test_write_noerror(writer, mpack_write_u8(writer, 149));
    test_write_noerror(writer, mpack_write_u8(writer, 157));
    test_write_noerror(writer, mpack_write_u8(writer, 173));
    test_write_noerror(writer, mpack_write_u8(writer, 191));
    test_write_noerror(writer, mpack_write_u8(writer, 199));
    test_write_noerror(writer, mpack_write_u8(writer, 223));
    test_write_noerror(writer, mpack_write_u8(writer, 227));

    test_write_noerror(writer, mpack_write_u16(writer, 257));
    test_write_noerror(writer, mpack_write_u16(writer, 7517));
    test_write_noerror(writer, mpack_write_u16(writer, 14767));
    test_write_noerror(writer, mpack_write_u16(writer, 22027));
    test_write_noerror(writer, mpack_write_u16(writer, 29269));
    test_write_noerror(writer, mpack_write_u16(writer, 36523));
    test_write_noerror(writer, mpack_write_u16(writer, 43777));
    test_write_noerror(writer, mpack_write_u16(writer, 51031));

    test_write_noerror(writer, mpack_write_u32(writer, 65537));
    test_write_noerror(writer, mpack_write_u32(writer, 477276851));
    test_write_noerror(writer, mpack_write_u32(writer, 954488153));
    test_write_noerror(writer, mpack_write_u32(writer, 1431699481));
    test_write_noerror(writer, mpack_write_u32(writer, 1908910763));

    // when using UINT32_C() and compiling the test suite as c++, gcc complains:
    // error: this decimal constant is unsigned only in ISO C90 [-Werror]
    test_write_noerror(writer, mpack_write_u32(writer, UINT64_C(2386122103)));
    test_write_noerror(writer, mpack_write_u32(writer, UINT64_C(2863333399)));
    test_write_noerror(writer, mpack_write_u32(writer, UINT64_C(3340544681)));

    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(4294967311)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(1941762537917555303)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(3883525071540143119)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(5825287605162730577)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(7767050138785318961)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(9708812672407906367)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(11650575206030493713)));
    test_write_noerror(writer, mpack_write_u64(writer, UINT64_C(13592337739653081091)));
}

static size_t test_buffer_fill(void* context, char* buffer, size_t count) {
    size_t* pos = (size_t*)context;
    size_t remaining = sizeof(test_buffer) - 1;
    if (remaining - *pos < count)
        count = remaining - *pos;
    memcpy(buffer, test_buffer + *pos, count);
    *pos += count;
    return count;
}

static void test_read_buffer(void) {
    for (size_t i = 0; i < sizeof(test_buffer_sizes) / sizeof(test_buffer_sizes[0]); ++i) {

        // initialize the reader with our buffer reader function
        mpack_reader_t reader;
        size_t size = test_buffer_sizes[i];
        char* buffer = (char*)malloc(size);
        size_t pos = 0;
        mpack_reader_init(&reader, buffer, size, 0);
        mpack_reader_set_fill(&reader, test_buffer_fill);
        mpack_reader_set_context(&reader, &pos);
        test_check_no_assertion();

        // read and destroy, ensuring no errors
        test_read_buffer_values(&reader);
        test_reader_destroy_noerror(&reader);
        free(buffer);

    }
}

// this test function doesn't bounds check its output; we just use a large
// enough output buffer for test purposes.
static bool test_buffer_flush(void* context, const char* buffer, size_t count) {
    char** pos = (char**)context;
    memcpy(*pos, buffer, count);
    *pos += count;
    return true;
}

static void test_write_buffer(void) {
    for (size_t i = 0; i < sizeof(test_buffer_sizes) / sizeof(test_buffer_sizes[0]); ++i) {
        char* output = (char*)malloc(0xffff);

        // initialize the writer with our buffer writer function
        mpack_writer_t writer;
        size_t size = test_buffer_sizes[i];
        char* buffer = (char*)malloc(size);
        char* pos = output;
        mpack_writer_init(&writer, buffer, size);
        mpack_writer_set_flush(&writer, test_buffer_flush);
        mpack_writer_set_context(&writer, &pos);
        test_check_no_assertion();

        // read and destroy, ensuring no errors
        test_write_buffer_values(&writer);
        test_writer_destroy_noerror(&writer);
        free(buffer);

        // check output
        test_assert(pos - output == sizeof(test_buffer) - 1,
                "output contains %i bytes but %i were expected",
                (int)(pos - output), (int)sizeof(test_buffer) - 1);
        test_assert(memcmp(output, test_buffer, sizeof(test_buffer) - 1) == 0,
                "output does not match test buffer");
        free(output);
    }
}

static void test_read_file(void) {
}

static void test_write_file(void) {
}

void test_buffers(void) {
    test_write_buffer();
    test_read_buffer();
    test_write_file();
    test_read_file();
}

