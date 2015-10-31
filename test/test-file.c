
#include "test-file.h"
#include "test-write.h"
#include "test-reader.h"
#include "test-node.h"

#if MPACK_STDIO

// the file tests currently all require the writer, since it
// is used to write the test data that is read back.
#if MPACK_WRITER

#ifdef WIN32
#define TEST_PATH "..\\..\\test\\"
#else
#include <unistd.h>
#define TEST_PATH "test/"
#endif

static const char* test_filename = "mpack-test-file";
static const char* test_dir = "mpack-test-dir";

static char* test_file_fetch(const char* filename, size_t* out_size) {
    *out_size = 0;

    // open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        test_assert(false, "missing file %s", filename);
        return NULL;
    }

    // get the file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) {
        test_assert(false, "invalid file size %i for %s", (int)size, filename);
        fclose(file);
        return NULL;
    }

    // allocate the data
    if (size == 0) {
        fclose(file);
        return (char*)MPACK_MALLOC(1);
    }
    char* data = (char*)MPACK_MALLOC(size);

    // read the file
    long total = 0;
    while (total < size) {
        size_t read = fread(data + total, 1, (size_t)(size - total), file);
        if (read <= 0) {
            test_assert(false, "failed to read from file %s", filename);
            fclose(file);
            MPACK_FREE(data);
            return NULL;
        }
        total += read;
    }

    fclose(file);
    *out_size = (size_t)size;
    return data;
}

static void test_file_write_bytes(mpack_writer_t* writer, mpack_tag_t tag) {
    mpack_write_tag(writer, tag);
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    for (; tag.v.l > sizeof(buf); tag.v.l -= (uint32_t)sizeof(buf))
        mpack_write_bytes(writer, buf, sizeof(buf));
    mpack_write_bytes(writer, buf, tag.v.l);
    mpack_finish_type(writer, tag.type);
}

static void test_file_write_elements(mpack_writer_t* writer, mpack_tag_t tag) {
    mpack_write_tag(writer, tag);
    for (size_t i = 0; i < tag.v.n; ++i) {
        if (tag.type == mpack_type_map)
            mpack_write_nil(writer);
        mpack_write_nil(writer);
    }
    mpack_finish_type(writer, tag.type);
}

static void test_file_write(void) {
    mpack_writer_t writer;
    mpack_writer_init_file(&writer, test_filename);
    test_assert(mpack_writer_error(&writer) == mpack_ok, "file open failed with %s",
            mpack_error_to_string(mpack_writer_error(&writer)));

    mpack_start_array(&writer, 5);

    // test compound types of various sizes

    mpack_start_array(&writer, 5);
    test_file_write_bytes(&writer, mpack_tag_str(0));
    test_file_write_bytes(&writer, mpack_tag_str(INT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_str(UINT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_str(UINT8_MAX + 1));
    test_file_write_bytes(&writer, mpack_tag_str(UINT16_MAX + 1));
    mpack_finish_array(&writer);

    mpack_start_array(&writer, 5);
    test_file_write_bytes(&writer, mpack_tag_bin(0));
    test_file_write_bytes(&writer, mpack_tag_bin(INT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_bin(UINT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_bin(UINT8_MAX + 1));
    test_file_write_bytes(&writer, mpack_tag_bin(UINT16_MAX + 1));
    mpack_finish_array(&writer);

    mpack_start_array(&writer, 10);
    test_file_write_bytes(&writer, mpack_tag_ext(1, 0));
    test_file_write_bytes(&writer, mpack_tag_ext(1, 1));
    test_file_write_bytes(&writer, mpack_tag_ext(1, 2));
    test_file_write_bytes(&writer, mpack_tag_ext(1, 4));
    test_file_write_bytes(&writer, mpack_tag_ext(1, 8));
    test_file_write_bytes(&writer, mpack_tag_ext(1, 16));
    test_file_write_bytes(&writer, mpack_tag_ext(2, INT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_ext(3, UINT8_MAX));
    test_file_write_bytes(&writer, mpack_tag_ext(4, UINT8_MAX + 1));
    test_file_write_bytes(&writer, mpack_tag_ext(5, UINT16_MAX + 1));
    mpack_finish_array(&writer);

    mpack_start_array(&writer, 5);
    test_file_write_elements(&writer, mpack_tag_array(0));
    test_file_write_elements(&writer, mpack_tag_array(INT8_MAX));
    test_file_write_elements(&writer, mpack_tag_array(UINT8_MAX));
    test_file_write_elements(&writer, mpack_tag_array(UINT8_MAX + 1));
    test_file_write_elements(&writer, mpack_tag_array(UINT16_MAX + 1));
    mpack_finish_array(&writer);

    mpack_start_array(&writer, 5);
    test_file_write_elements(&writer, mpack_tag_map(0));
    test_file_write_elements(&writer, mpack_tag_map(INT8_MAX));
    test_file_write_elements(&writer, mpack_tag_map(UINT8_MAX));
    test_file_write_elements(&writer, mpack_tag_map(UINT8_MAX + 1));
    test_file_write_elements(&writer, mpack_tag_map(UINT16_MAX + 1));
    mpack_finish_array(&writer);

    mpack_finish_array(&writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    test_assert(error == mpack_ok, "write failed with %s", mpack_error_to_string(error));

    // test invalid filename
    mkdir(test_dir, 0700);
    mpack_writer_init_file(&writer, test_dir);
    test_writer_destroy_error(&writer, mpack_error_io);

    // test close and flush failure
    // (if we write more than libc's internal FILE buffer size, fwrite()
    // fails, otherwise fclose() fails. we test both here.)

    mpack_writer_init_file(&writer, "/dev/full");
    mpack_write_cstr(&writer, "The quick brown fox jumps over the lazy dog.");
    test_writer_destroy_error(&writer, mpack_error_io);

    int count = UINT16_MAX / 20;
    mpack_writer_init_file(&writer, "/dev/full");
    mpack_start_array(&writer, count);
    for (int i = 0; i < count; ++i)
        mpack_write_cstr(&writer, "The quick brown fox jumps over the lazy dog.");
    mpack_finish_array(&writer);
    test_writer_destroy_error(&writer, mpack_error_io);
}

static bool test_file_write_failure() {

    // The write failure test may fail with either
    // mpack_error_memory or mpack_error_io. We write a
    // bunch of strs and bins to test the various expect
    // allocator modes.

    mpack_writer_t writer;
    mpack_writer_init_file(&writer, test_filename);

    mpack_start_array(&writer, 6);

    // write a large string near the start to cause a
    // more than double buffer size growth
    mpack_write_cstr(&writer, "The quick brown fox jumps over a lazy dog.");

    mpack_write_cstr(&writer, "one");
    mpack_write_cstr(&writer, "two");
    mpack_write_cstr(&writer, "three");
    mpack_write_cstr(&writer, "four");
    mpack_write_cstr(&writer, "five");
    mpack_finish_array(&writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    if (error == mpack_error_io || error == mpack_error_memory)
        return false;
    test_assert(error == mpack_ok, "unexpected error state %i (%s)", (int)error,
            mpack_error_to_string(error));
    return true;

}

// compares the test filename to the expected debug output
static void test_compare_print() {
    size_t expected_size;
    char* expected_data = test_file_fetch(TEST_PATH "test-file.debug", &expected_size);
    size_t actual_size;
    char* actual_data = test_file_fetch(test_filename, &actual_size);

    test_assert(actual_size == expected_size, "print length %i does not match expected length %i",
            (int)actual_size, (int)expected_size);
    test_assert(0 == memcmp(actual_data, expected_data, actual_size), "print does not match expected");

    MPACK_FREE(expected_data);
    MPACK_FREE(actual_data);
}

#if MPACK_READER
static void test_print(void) {

    // miscellaneous print tests
    // (we're not actually checking the output; we just want to make
    // sure it doesn't crash under the below errors.)
    FILE* out = fopen(test_filename, "wb");
    mpack_print_file("\x91", 1, out); // truncated file
    mpack_print_file("\xa1", 1, out); // truncated str
    mpack_print_file("\x92\x00", 2, out); // truncated array
    mpack_print_file("\x81", 1, out); // truncated map key
    mpack_print_file("\x81\x00", 2, out); // truncated map value
    mpack_print_file("\x90\xc0", 2, out); // extra bytes
    mpack_print_file("\xca\x00\x00\x00\x00", 5, out); // float
    fclose(out);


    // dump MessagePack to debug file

    size_t input_size;
    char* input_data = test_file_fetch(TEST_PATH "test-file.mp", &input_size);

    out = fopen(test_filename, "wb");
    mpack_print_file(input_data, input_size, out);
    fclose(out);

    MPACK_FREE(input_data);
    test_compare_print();
}
#endif

#if MPACK_NODE
static void test_node_print(void) {
    mpack_tree_t tree;

    // miscellaneous node print tests
    FILE* out = fopen(test_filename, "wb");
    mpack_tree_init(&tree, "\xca\x00\x00\x00\x00", 5); // float
    mpack_node_print_file(mpack_tree_root(&tree), out);
    mpack_tree_destroy(&tree);
    fclose(out);


    // dump MessagePack to debug file

    mpack_tree_init_file(&tree, TEST_PATH "test-file.mp", 0);

    out = fopen(test_filename, "wb");
    mpack_node_print_file(mpack_tree_root(&tree), out);
    fclose(out);

    test_assert(mpack_ok == mpack_tree_destroy(&tree));
    test_compare_print();
}
#endif

#if MPACK_READER
static void test_file_discard(void) {
    mpack_reader_t reader;
    mpack_reader_init_file(&reader, test_filename);
    mpack_discard(&reader);
    mpack_error_t error = mpack_reader_destroy(&reader);
    test_assert(error == mpack_ok, "read failed with %s", mpack_error_to_string(error));
}
#endif

#if MPACK_EXPECT
static void test_file_expect_bytes(mpack_reader_t* reader, mpack_tag_t tag) {
    mpack_expect_tag(reader, tag);

    char expected[1024];
    memset(expected, 0, sizeof(expected));
    char buf[sizeof(expected)];
    while (tag.v.l > 0) {
        uint32_t read = (tag.v.l < (uint32_t)sizeof(buf)) ? tag.v.l : (uint32_t)sizeof(buf);
        mpack_read_bytes(reader, buf, read);
        test_assert(mpack_reader_error(reader) == mpack_ok);
        test_assert(memcmp(buf, expected, read) == 0);
        tag.v.l -= read;
    }

    mpack_done_type(reader, tag.type);
}

static void test_file_expect_elements(mpack_reader_t* reader, mpack_tag_t tag) {
    mpack_expect_tag(reader, tag);
    for (size_t i = 0; i < tag.v.l; ++i) {
        if (tag.type == mpack_type_map)
            mpack_expect_nil(reader);
        mpack_expect_nil(reader);
    }
    mpack_done_type(reader, tag.type);
}

static void test_file_read(void) {
    mpack_reader_t reader;
    mpack_reader_init_file(&reader, test_filename);
    test_assert(mpack_reader_error(&reader) == mpack_ok, "file open failed with %s",
            mpack_error_to_string(mpack_reader_error(&reader)));

    test_assert(5 == mpack_expect_array(&reader));

    test_assert(5 == mpack_expect_array(&reader));
    test_file_expect_bytes(&reader, mpack_tag_str(0));
    test_file_expect_bytes(&reader, mpack_tag_str(INT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_str(UINT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_str(UINT8_MAX + 1));
    test_file_expect_bytes(&reader, mpack_tag_str(UINT16_MAX + 1));
    mpack_done_array(&reader);

    test_assert(5 == mpack_expect_array(&reader));
    test_file_expect_bytes(&reader, mpack_tag_bin(0));
    test_file_expect_bytes(&reader, mpack_tag_bin(INT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_bin(UINT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_bin(UINT8_MAX + 1));
    test_file_expect_bytes(&reader, mpack_tag_bin(UINT16_MAX + 1));
    mpack_done_array(&reader);

    test_assert(10 == mpack_expect_array(&reader));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 0));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 1));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 2));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 4));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 8));
    test_file_expect_bytes(&reader, mpack_tag_ext(1, 16));
    test_file_expect_bytes(&reader, mpack_tag_ext(2, INT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_ext(3, UINT8_MAX));
    test_file_expect_bytes(&reader, mpack_tag_ext(4, UINT8_MAX + 1));
    test_file_expect_bytes(&reader, mpack_tag_ext(5, UINT16_MAX + 1));
    mpack_done_array(&reader);

    test_assert(5 == mpack_expect_array(&reader));
    test_file_expect_elements(&reader, mpack_tag_array(0));
    test_file_expect_elements(&reader, mpack_tag_array(INT8_MAX));
    test_file_expect_elements(&reader, mpack_tag_array(UINT8_MAX));
    test_file_expect_elements(&reader, mpack_tag_array(UINT8_MAX + 1));
    test_file_expect_elements(&reader, mpack_tag_array(UINT16_MAX + 1));
    mpack_done_array(&reader);

    test_assert(5 == mpack_expect_array(&reader));
    test_file_expect_elements(&reader, mpack_tag_map(0));
    test_file_expect_elements(&reader, mpack_tag_map(INT8_MAX));
    test_file_expect_elements(&reader, mpack_tag_map(UINT8_MAX));
    test_file_expect_elements(&reader, mpack_tag_map(UINT8_MAX + 1));
    test_file_expect_elements(&reader, mpack_tag_map(UINT16_MAX + 1));
    mpack_done_array(&reader);

    mpack_done_array(&reader);

    mpack_error_t error = mpack_reader_destroy(&reader);
    test_assert(error == mpack_ok, "read failed with %s", mpack_error_to_string(error));

    // test missing file
    mpack_reader_init_file(&reader, "invalid-filename");
    test_reader_destroy_error(&reader, mpack_error_io);
}

static bool test_file_expect_failure() {

    // The write failure test may fail with either
    // mpack_error_memory or mpack_error_io. We write a
    // bunch of strs and bins to test the various expect
    // allocator modes.

    mpack_reader_t reader;

    #define TEST_POSSIBLE_FAILURE() do { \
        mpack_error_t error = mpack_reader_error(&reader); \
        if (error == mpack_error_memory || error == mpack_error_io) { \
            mpack_reader_destroy(&reader); \
            return false; \
        } \
    } while (0)

    mpack_reader_init_file(&reader, test_filename);

    uint32_t count;
    char** strings = mpack_expect_array_alloc(&reader, char*, 50, &count);
    TEST_POSSIBLE_FAILURE();
    test_assert(strings != NULL);
    test_assert(count == 6);
    MPACK_FREE(strings);

    size_t size;
    char* str = mpack_expect_str_alloc(&reader, 100, &size);
    TEST_POSSIBLE_FAILURE();
    test_assert(str != NULL);
    const char* expected = "The quick brown fox jumps over a lazy dog.";
    test_assert(size == strlen(expected));
    test_assert(memcmp(str, expected, size) == 0);
    MPACK_FREE(str);

    str = mpack_expect_utf8_alloc(&reader, 100, &size);
    TEST_POSSIBLE_FAILURE();
    test_assert(str != NULL);
    expected = "one";
    test_assert(size == strlen(expected));
    test_assert(memcmp(str, expected, size) == 0);
    MPACK_FREE(str);

    str = mpack_expect_cstr_alloc(&reader, 100);
    TEST_POSSIBLE_FAILURE();
    test_assert(str != NULL);
    test_assert(strcmp(str, "two") == 0);
    MPACK_FREE(str);

    str = mpack_expect_utf8_cstr_alloc(&reader, 100);
    TEST_POSSIBLE_FAILURE();
    test_assert(str != NULL);
    test_assert(strcmp(str, "three") == 0);
    MPACK_FREE(str);

    mpack_discard(&reader);
    mpack_discard(&reader);

    mpack_done_array(&reader);

    #undef TEST_POSSIBLE_FAILURE

    mpack_error_t error = mpack_reader_destroy(&reader);
    if (error == mpack_error_io || error == mpack_error_memory)
        return false;
    test_assert(error == mpack_ok, "unexpected error state %i (%s)", (int)error,
            mpack_error_to_string(error));
    return true;

}
#endif

#if MPACK_NODE
static void test_file_node_bytes(mpack_node_t node, mpack_tag_t tag) {
    test_assert(mpack_tag_equal(tag, mpack_node_tag(node)));
    const char* data = mpack_node_data(node);
    uint32_t length = mpack_node_data_len(node);
    test_assert(mpack_node_error(node) == mpack_ok);

    char expected[1024];
    memset(expected, 0, sizeof(expected));
    while (length > 0) {
        uint32_t count = (length < (uint32_t)sizeof(expected)) ? length : (uint32_t)sizeof(expected);
        test_assert(memcmp(data, expected, count) == 0);
        length -= count;
        data += count;
    }
}

static void test_file_node_elements(mpack_node_t node, mpack_tag_t tag) {
    test_assert(mpack_tag_equal(tag, mpack_node_tag(node)));
    for (size_t i = 0; i < tag.v.n; ++i) {
        if (tag.type == mpack_type_map) {
            mpack_node_nil(mpack_node_map_key_at(node, i));
            mpack_node_nil(mpack_node_map_value_at(node, i));
        } else {
            mpack_node_nil(mpack_node_array_at(node, i));
        }
    }
}

static void test_file_node(void) {
    mpack_tree_t tree;
    mpack_tree_init_file(&tree, test_filename, 0);
    test_assert(mpack_tree_error(&tree) == mpack_ok, "file tree parsing failed: %s",
            mpack_error_to_string(mpack_tree_error(&tree)));

    mpack_node_t root = mpack_tree_root(&tree);
    test_assert(mpack_node_array_length(root) == 5);

    mpack_node_t node = mpack_node_array_at(root, 0);
    test_assert(mpack_node_array_length(node) == 5);
    test_file_node_bytes(mpack_node_array_at(node, 0), mpack_tag_str(0));
    test_file_node_bytes(mpack_node_array_at(node, 1), mpack_tag_str(INT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 2), mpack_tag_str(UINT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 3), mpack_tag_str(UINT8_MAX + 1));
    test_file_node_bytes(mpack_node_array_at(node, 4), mpack_tag_str(UINT16_MAX + 1));

    node = mpack_node_array_at(root, 1);
    test_assert(5 == mpack_node_array_length(node));
    test_file_node_bytes(mpack_node_array_at(node, 0), mpack_tag_bin(0));
    test_file_node_bytes(mpack_node_array_at(node, 1), mpack_tag_bin(INT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 2), mpack_tag_bin(UINT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 3), mpack_tag_bin(UINT8_MAX + 1));
    test_file_node_bytes(mpack_node_array_at(node, 4), mpack_tag_bin(UINT16_MAX + 1));

    node = mpack_node_array_at(root, 2);
    test_assert(10 == mpack_node_array_length(node));
    test_file_node_bytes(mpack_node_array_at(node, 0), mpack_tag_ext(1, 0));
    test_file_node_bytes(mpack_node_array_at(node, 1), mpack_tag_ext(1, 1));
    test_file_node_bytes(mpack_node_array_at(node, 2), mpack_tag_ext(1, 2));
    test_file_node_bytes(mpack_node_array_at(node, 3), mpack_tag_ext(1, 4));
    test_file_node_bytes(mpack_node_array_at(node, 4), mpack_tag_ext(1, 8));
    test_file_node_bytes(mpack_node_array_at(node, 5), mpack_tag_ext(1, 16));
    test_file_node_bytes(mpack_node_array_at(node, 6), mpack_tag_ext(2, INT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 7), mpack_tag_ext(3, UINT8_MAX));
    test_file_node_bytes(mpack_node_array_at(node, 8), mpack_tag_ext(4, UINT8_MAX + 1));
    test_file_node_bytes(mpack_node_array_at(node, 9), mpack_tag_ext(5, UINT16_MAX + 1));

    node = mpack_node_array_at(root, 3);
    test_assert(5 == mpack_node_array_length(node));
    test_file_node_elements(mpack_node_array_at(node, 0), mpack_tag_array(0));
    test_file_node_elements(mpack_node_array_at(node, 1), mpack_tag_array(INT8_MAX));
    test_file_node_elements(mpack_node_array_at(node, 2), mpack_tag_array(UINT8_MAX));
    test_file_node_elements(mpack_node_array_at(node, 3), mpack_tag_array(UINT8_MAX + 1));
    test_file_node_elements(mpack_node_array_at(node, 4), mpack_tag_array(UINT16_MAX + 1));

    node = mpack_node_array_at(root, 4);
    test_assert(5 == mpack_node_array_length(node));
    test_file_node_elements(mpack_node_array_at(node, 0), mpack_tag_map(0));
    test_file_node_elements(mpack_node_array_at(node, 1), mpack_tag_map(INT8_MAX));
    test_file_node_elements(mpack_node_array_at(node, 2), mpack_tag_map(UINT8_MAX));
    test_file_node_elements(mpack_node_array_at(node, 3), mpack_tag_map(UINT8_MAX + 1));
    test_file_node_elements(mpack_node_array_at(node, 4), mpack_tag_map(UINT16_MAX + 1));

    mpack_error_t error = mpack_tree_destroy(&tree);
    test_assert(error == mpack_ok, "file tree failed with error %s", mpack_error_to_string(error));

    // test file size out of bounds
    if (sizeof(size_t) >= sizeof(long)) {
        test_expecting_break((mpack_tree_init_file(&tree, "invalid-filename", ((size_t)LONG_MAX) + 1), true));
        test_tree_destroy_error(&tree, mpack_error_bug);
    }

    // test missing file
    mpack_tree_init_file(&tree, "invalid-filename", 0);
    test_tree_destroy_error(&tree, mpack_error_io);
}
#endif

void test_file(void) {
    #if MPACK_READER
    test_print();
    #endif
    #if MPACK_NODE
    test_node_print();
    #endif

    test_file_write();

    #if MPACK_READER
    test_file_discard();
    #endif
    #if MPACK_EXPECT
    test_file_read();
    #endif
    #if MPACK_NODE
    test_file_node();
    #endif

    test_system_fail_until_ok(&test_file_write_failure);
    #if MPACK_EXPECT
    test_system_fail_until_ok(&test_file_expect_failure);
    #endif

    test_assert(remove(test_filename) == 0, "failed to delete %s", test_filename);
    test_assert(rmdir(test_dir) == 0, "failed to delete %s", test_dir);

    (void)&test_compare_print;
}

#else

void test_file(void) {
    // if we don't have the writer, nothing to do
}

#endif
#endif

