# Using the Reader API

The Reader API is used to parse MessagePack incrementally with no per-element memory usage. Elements can be read one-by-one, and the content of strings and binary blobs can be read in chunks.

Reading incrementally is much more difficult and verbose than using the [Node API](docs/node.md). If you are not constrained in memory or performance, you should use the Node API.

## The Basics

A reader is first initialized against a data source. This can be a chunk of data in memory, or it can be a file or stream. A reader that uses a file or stream will read data in chunks into an internal buffer. This allows parsing very large messages efficiently with minimal memory usage.

Once initialized, the fundamental operation of a reader is to read a tag. A tag is a value struct of type `mpack_tag_t` that contains the value of a single element, or the metadata for a compound element.

Here's a minimal example that parses the first element out of a chunk of MessagePack data.

```C
bool parse_first_element(const char* data, size_t length) {
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, length);

    mpack_tag_t tag = mpack_read_tag(&reader);
    do_something_with_tag(&tag);

    return mpack_reader_destroy(&reader) == mpack_ok;
};
```

The struct `mpack_tag_t` contains the element type, accessible with `mpack_tag_type()`. Depending on the type, you can access the value with tag accessor functions such as `mpack_tag_bool_value()` or `mpack_tag_array_count()`.

Of course, parsing single values isn't terribly useful. You'll need to know how to parse strings, maps and arrays.

## Compound Types

Compound types are either containers (map, array) or data chunks (strings, binary blobs). For any compound type, the tag only contains the compound type's size. You still need to read the contained data, and then you need to let the reader know that you're done reading the element (so it can check that you did it correctly.)

### Containers

To parse a container, you must read as many additional elements as are specified by the tag's count. Note that a map tag specifies the number of key value pairs it contains, so you must actually read double the tag's count. You must then call `mpack_done_array()` or `mpack_done_map()` so that the reader can verify that you read the correct number of elements.

Here's an example of a function that reads a tag from a reader, then reads all of the contained elements recursively:

```C
void parse_element(mpack_reader_t* reader, int depth) {
    if (depth >= 32) { // critical check!
        mpack_reader_flag_error(reader, mpack_error_too_big);
        return;
    }

    mpack_tag_t tag = mpack_read_tag(&reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return;

    do_something_with_tag(&tag);

    if (mpack_tag_type(&tag) == mpack_type_array) {
        for (uint32_t i = mpack_tag_array_count(&tag); i > 0; --i) {
            parse_element(reader, depth + 1);
            if (mpack_reader_error(reader) != mpack_ok) // critical check!
                break;
        }
        mpack_done_array(reader);
    }

    if (mpack_tag_type(&tag) == mpack_type_map) {
        for (uint32_t i = mpack_tag_map_count(&tag); i > 0; --i) {
            parse_element(reader, depth + 1);
            parse_element(reader, depth + 1);
            if (mpack_reader_error(reader) != mpack_ok) // critical check!
                break;
        }
        mpack_done_map(reader);
    }
}
```

WARNING: It is critical that we check for errors during each iteration of our array and map loops. If we skip these checks, malicious data could claim to contain billions of elements, throwing us into a very long loop. These checks allow us to break out as soon as we run out of data. Since this function is recursive, we've also added a depth limit to prevent bad data from causing a stack overflow.

(The [Node API](docs/node.md) is not vulnerable to such problems. In fact, it can break out of such bad data even sooner. The Node API will automatically flag an error as soon as it realizes that there aren't enough bytes left in the message to fill the remaining elements of all open compound types. If possible, consider using the Node API.)

Unfortunately, the above code has a problem: it does not handle strings or binary blobs. If it encounters them, it will fail because it's not reading their contents.

### Compound Data Chunks

The data for chunks such as strings and binary blobs must be read separately. Bytes can be read across multiple read calls, as long the total number of bytes you read matches the number of bytes contained in the element. And as with containers, you must call `mpack_done_str()` or `mpack_done_bin()` so that the reader can verify that you read the correct total number of bytes.

We can add support for strings to the previous example with the below code. In this example, we read the string incrementally in 128 byte chunks to keep memory usage at an absolute minimum:

```C
    if (mpack_tag_type(&tag) == mpack_type_str) {
        uint32_t length = mpack_tag_str_length(&tag);
        char buffer[128];

        while (length > 0) {
            size_t step = (length < sizeof(buffer)) ? length : sizeof(buffer);
            mpack_read_bytes(reader, buffer, sizeof(buffer);
            if (mpack_reader_error(reader) != mpack_ok) // critical check!
                break;

            do_something_with_partial_string(buffer, step);
        }

        mpack_done_str(reader);
    }
```

As above, we need a safety check to ensure that bad data cannot get us stuck in a loop.

Code for `mpack_type_bin` is identical except that the type and function names contain `bin` instead of `str`.

### In-Place strings

The MPack reader supports accessing the data contained in compound data types directly out of the reader's buffer. This can provide zero-copy access to strings and binary blobs, provided they fit within the buffer.

To access string or bin data in-place, use `mpack_read_bytes_inplace()` (or `mpack_read_utf8_inplace()` to also check a string's UTF-8 encoding.) This provides a pointer directly into the reader's buffer, moving data around as necessary to make it fit. The previous code could be replaced with:

```C
    if (mpack_tag_type(&tag) == mpack_type_str) {
        uint32_t length = mpack_tag_str_length(&tag);
        const char* data = mpack_read_bytes_inplace(reader, length);
        if (mpack_reader_error(reader) != mpack_ok)
            return;
        do_something_with_string(data, length);
        mpack_done_str(reader);
    }
```

There's an important caveat here though: this will flag an error if the string does not fit in the buffer. If you are decoding a chunk of MessagePack data in memory (without a fill function), then this is not a problem, as it would simply mean that the message was truncated. But if you are decoding from a file or stream, you need to account for the fact that the string may not fit in the buffer.

To work around this, you can provide two paths for reading data depending on the size of the string. MPack provides a helper `mpack_should_read_bytes_inplace()` to tell you if it's a good idea to read in-place. Here's another example that uses zero-copy strings where possible, and falls back to an allocation otherwise:

```C
    if (mpack_tag_type(&tag) == mpack_type_str) {
        uint32_t length = mpack_tag_str_length(&tag);
        if (length >= 16 * 1024) { // critical check! limit length to avoid a huge allocation
            mpack_reader_flag_error(reader, mpack_error_too_big);
            return;
        }

        if (mpack_should_read_bytes_inplace(reader, length)) {
            const char* data = mpack_read_bytes_inplace(reader, length);
            if (mpack_reader_error(reader) != mpack_ok)
                return;
            do_something_with_string(data, length);

        } else {
            char* data = malloc(length);
            mpack_read_bytes(reader, data, length);
            if (mpack_reader_error(reader) != mpack_ok) {
                free(data);
                return;
            }
            do_something_with_string(data, length);
            free(data);
        }

        mpack_done_str(reader);
    }
```

## Expected Types

When parsing data of expected types from a dynamic stream, you will likely want to hardcode type checks before each access. For example:

```C
// get a bool
mpack_tag_t tag = mpack_read_tag(reader);
if (mpack_tag_type(&tag) != mpack_type_bool) {
    perror("not a bool!");
    return false;
}
bool value = mpack_tag_bool_value(&tag);
```

MPack provides helper functions that perform these checks in the [Expect API](docs/expect.md). The above code for example could be replaced by a single call to `mpack_expect_bool()`. If the value is not a bool, it will flag an error and return `false`. This is the incremental reader analogue to `mpack_write_bool()`.

These helpers are strongly recommended because they will perform range checks and lossless conversions where needed. For example, suppose you want to read an `int`. A positive integer could be stored in a tag as two different types (`mpack_type_int` or `mpack_type_uint`), and the value of that integer could be outside the range of an `int`. The functions `mpack_expect_int()` supports both types and performs the appropriate range checks. It will convert losslessly as needed and flag an error if the value doesn't fit.

Note that when using the expect functions, you still need to read the contents of compound types and call the corresponding done function. A call to an expect function only replaces a call to `mpack_read_tag()`. For example:

```C
uint32_t length = mpack_expect_str(reader);
const char* data = mpack_read_bytes_inplace(reader, length);
if (mpack_reader_error(reader) == mpack_ok)
    do_something_with_string(data, length);
mpack_done_str(reader);
```

If you want to always check that types match what you expect, head on over to the [Expect API](docs/expect.md) and use these type checking functions. But if you are still interested in handling the dynamic runtime type of elements, read on.

## Event-Based Parser

In this example, we'll implement a SAX-style event-based parser for blobs of MessagePack using the Reader API. This is a good example of non-trivial MPack usage that handles the dynamic type of incrementally parsed MessagePack data. It's easy to convert any incremental parser to an event-based parser, as we'll soon see.

First let's define a header file with a list of callbacks for our parser, and our single function to launch the parser:

```C
typedef struct sax_callbacks_t {
    void (*nil_element)(void* context);
    void (*bool_element)(void* context, int64_t value);
    void (*int_element)(void* context, int64_t value);
    void (*uint_element)(void* context, uint64_t value);
    void (*string_element)(void* context, const char* data, uint32_t length);

    void (*start_map)(void* context, uint32_t pair_count);
    void (*start_array)(void* context, uint32_t element_count);
    void (*finish_map)(void* context);
    void (*finish_array)(void* context);
} sax_callbacks_t;

/**
 * Parse a blob of MessagePack data, calling the appropriate callback for each
 * element encountered.
 *
 * @return true if successful, false if any error occurs.
 */
bool parse_messagepack(const char* data, size_t length,
        const sax_callbacks_t* callbacks, void* context);
```

Next we'll define our `parse_messagepack()` function in a corresponding source file to set our our reader. This will wrap another function called `parse_element()`.

```C
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
```

We'll make `parse_element()` recursive to keep things simple. This makes it extremely straightforward to implement. We just parse a tag, switch on the type, and call the appropriate callback function.

Note that we can access all strings in-place because the source data is a single chunk of contiguous memory.

```C
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
```

As above, the error checks within loops are critical to keep the parser safe against untrusted data.

This is all that is needed to convert MPack into an event-based parser.
