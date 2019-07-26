#include "sax-example.h"

static void parse_element(mpack_reader_t* reader, int depth,
        const sax_callbacks_t* callbacks, void* context);

bool parse_messagepack(const char* data, size_t length,
        const sax_callbacks_t* callbacks, void* context)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, length);
    parse_element(&reader, 0, callbacks, context);
    return mpack_ok == mpack_reader_destroy(&reader);
}

static void parse_element(mpack_reader_t* reader, int depth,
        const sax_callbacks_t* callbacks, void* context)
{
    if (depth >= 32) { // critical check!
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return;
    }

    mpack_tag_t tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return;

    switch (mpack_tag_type(&tag)) {
        case mpack_type_nil:
            callbacks->nil_element(context);
            break;
        case mpack_type_bool:
            callbacks->bool_element(context, mpack_tag_bool_value(&tag));
            break;
        case mpack_type_int:
            callbacks->int_element(context, mpack_tag_int_value(&tag));
            break;
        case mpack_type_uint:
            callbacks->uint_element(context, mpack_tag_uint_value(&tag));
            break;

        case mpack_type_str: {
            uint32_t length = mpack_tag_str_length(&tag);
            const char* data = mpack_read_bytes_inplace(reader, length);
            callbacks->string_element(context, data, length);
            mpack_done_str(reader);
            break;
        }

        case mpack_type_array: {
            uint32_t count = mpack_tag_array_count(&tag);
            callbacks->start_array(context, count);
            while (count-- > 0) {
                parse_element(reader, depth + 1, callbacks, context);
                if (mpack_reader_error(reader) != mpack_ok) // critical check!
                    break;
            }
            callbacks->finish_array(context);
            mpack_done_array(reader);
            break;
        }

        case mpack_type_map: {
            uint32_t count = mpack_tag_map_count(&tag);
            callbacks->start_map(context, count);
            while (count-- > 0) {
                parse_element(reader, depth + 1, callbacks, context);
                parse_element(reader, depth + 1, callbacks, context);
                if (mpack_reader_error(reader) != mpack_ok) // critical check!
                    break;
            }
            callbacks->finish_map(context);
            mpack_done_map(reader);
            break;
        }

        default:
            mpack_reader_flag_error(reader, mpack_error_unsupported);
            break;
    }
}

#define SAX_EXAMPLE_TEST 1
#if SAX_EXAMPLE_TEST
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static void nil_element(void* context) {
    printf("nil\n");
}

static void bool_element(void* context, int64_t value) {
    printf("bool: %s\n", value ? "true" : "false");
}

static void int_element(void* context, int64_t value) {
    printf("int: %" PRIi64 "\n", value);
}

static void uint_element(void* context, uint64_t value) {
    printf("uint: %" PRIu64 "\n", value);
}

static void string_element(void* context, const char* data, uint32_t length) {
    printf("string: \"");
    fwrite(data, 1, length, stdout);
    printf("\"\n");
}

static void start_map(void* context, uint32_t pair_count) {
    printf("starting map of %u key-value pairs\n", pair_count);
}

static void start_array(void* context, uint32_t element_count) {
    printf("starting array of %u key-value pairs\n", element_count);
}

static void finish_map(void* context) {
    printf("finishing map\n");
}

static void finish_array(void* context) {
    printf("finishing array\n");
}

static sax_callbacks_t callbacks = {
    nil_element,
    bool_element,
    int_element,
    uint_element,
    string_element,
    start_map,
    start_array,
    finish_map,
    finish_array,
};

int main(int argc, char** argv) {
    struct stat stat;
    int fd = open(argv[1], O_RDONLY);
    fstat(fd, &stat);
    const char* p = (const char*) mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    bool ok = parse_messagepack(p, stat.st_size, &callbacks, NULL);
    if (!ok)
        printf("error!\n");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
