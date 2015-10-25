
#include "test-file.h"
#include "test-write.h"
#include "test-reader.h"
#include "test-node.h"

#if MPACK_STDIO

// the file tests currently all require the writer, since it
// is used to write the test data that is read back.
#if MPACK_WRITER

#ifndef WIN32
#include <unistd.h>
#endif

static const char* test_filename = "mpack-test-file.mp";
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
    for (size_t i = 0; i < tag.v.l; ++i)
        mpack_write_bytes(writer, "", 1);
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

// compares the test filename to the expected debug output
static void test_compare_print() {
    size_t expected_size;
    char* expected_data = test_file_fetch("test/test-file.debug", &expected_size);
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
    size_t input_size;
    char* input_data = test_file_fetch("test/test-file.mp", &input_size);

    FILE* out = fopen(test_filename, "wb");
    mpack_print_file(input_data, input_size, out);
    fclose(out);

    MPACK_FREE(input_data);
    test_compare_print();
}
#endif

#if MPACK_NODE
static void test_node_print(void) {
    mpack_tree_t tree;
    mpack_tree_init_file(&tree, "test/test-file.mp", 0);

    FILE* out = fopen(test_filename, "wb");
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
    for (size_t i = 0; i < tag.v.l; ++i) {
        char buf[1];
        mpack_read_bytes(reader, buf, 1);
        test_assert(buf[0] == 0);
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
#endif

#if MPACK_NODE
static void test_file_node_bytes(mpack_node_t node, mpack_tag_t tag) {
    test_assert(mpack_tag_equal(tag, mpack_node_tag(node)));
    const char* data = mpack_node_data(node);
    for (size_t i = 0; i < tag.v.l; ++i)
        test_assert(data[i] == 0);
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

    // test unseekable stream
    mpack_tree_init_file(&tree, "/dev/zero", 0);
    test_tree_destroy_error(&tree, mpack_error_invalid);

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

    test_assert(remove(test_filename) == 0, "failed to delete %s", test_filename);
    test_assert(rmdir(test_dir) == 0, "failed to delete %s", test_dir);

    (void)test_compare_print;
}

#else

void test_file(void) {
    // if we don't have the writer, nothing to do
}

#endif
#endif

