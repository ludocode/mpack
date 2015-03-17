
#include "test-file.h"

#include <unistd.h>

static const char* test_filename = "testfile.mp";

static void test_file_write(void) {
    mpack_writer_t writer;
    mpack_writer_init_file(&writer, test_filename);
    test_assert(mpack_writer_error(&writer) == mpack_ok, "file open failed with %s",
            mpack_error_to_string(mpack_writer_error(&writer)));

    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "compact");
    mpack_write_bool(&writer, true);
    mpack_write_cstr(&writer, "schema");
    mpack_write_int(&writer, 0);
    mpack_finish_map(&writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    test_assert(error == mpack_ok, "write failed with %s", mpack_error_to_string(error));
}

static void test_file_check(void) {
    static const char test[] = "\x82\xA7""compact\xC3\xA6""schema\x00";

    FILE* file = fopen(test_filename, "rb");
    test_assert(file != NULL, "file open failed");
    char buffer[1024];
    size_t count = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    test_assert(count == sizeof(test) - 1, "data is incorrect length, read %i expected %i",
            (int)count, (int)(sizeof(test) - 1));
    test_assert(memcmp(buffer, test, count) == 0, "data does not match");
}

static void test_file_read(void) {
    mpack_reader_t reader;
    mpack_reader_init_file(&reader, test_filename);
    test_assert(mpack_reader_error(&reader) == mpack_ok, "file open failed with %s",
            mpack_error_to_string(mpack_reader_error(&reader)));

    test_assert(2 == mpack_expect_map(&reader));
    mpack_expect_cstr_match(&reader, "compact");
    test_assert(true == mpack_expect_bool(&reader));
    mpack_expect_cstr_match(&reader, "schema");
    test_assert(0 == mpack_expect_u8(&reader));
    mpack_done_map(&reader);

    mpack_error_t error = mpack_reader_destroy(&reader);
    test_assert(error == mpack_ok, "read failed with %s", mpack_error_to_string(error));
}

static void test_file_node(void) {
    mpack_tree_t tree;
    mpack_tree_init_file(&tree, test_filename, 0);
    test_assert(mpack_tree_error(&tree) == mpack_ok, "file tree parsing failed: %s",
            mpack_error_to_string(mpack_tree_error(&tree)));

    mpack_node_t* root = mpack_tree_root(&tree);
    test_assert(mpack_node_map_count(root) == 2, "map contains %i keys", (int)mpack_node_map_count(root));
    test_assert(mpack_node_i8(mpack_node_map_cstr(root, "schema")) == 0, "checking schema failed");
    test_assert(mpack_node_bool(mpack_node_map_cstr(root, "compact")) == true, "checking compact failed");

    mpack_error_t error = mpack_tree_destroy(&tree);
    test_assert(error == mpack_ok, "file tree failed with error %s", mpack_error_to_string(error));
}

void test_file(void) {
    test_file_write();
    test_file_check();
    test_file_read();
    test_file_node();
    unlink(test_filename);
}

