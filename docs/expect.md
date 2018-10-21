# Using the Expect API

The Expect API is used to imperatively parse data of a fixed (hardcoded) schema. It is most useful when parsing very large MessagePack files, parsing in memory-constrained environments, or generating parsing code from a schema. The API is similar to [CMP](https://github.com/camgunz/cmp), but has many helper functions especially for map keys and expected value ranges. Some of these will be covered below.

Check out the [Reader API](docs/reader.md) guide first for information on setting up a reader and reading strings.

If you are not writing code for an embedded device or generating parsing code from a schema, you should not follow this guide. You should most likely be using the [Node API](docs/node.md) instead.

## A simple example

Suppose we have data that we know will have the following schema:

```
an array containing three elements
  a UTF-8 string of at most 127 characters
  a UTF-8 string of at most 127 characters
  an array containing up to ten elements
    where all elements are ints
```

For example, we could have the following bytes in a MessagePack file called `example.mp`:

```Shell
93                     # an array containing three elements
  a5 68 65 6c 6c 6f    # "hello"
  a6 77 6f 72 6c 64 21 # "world!"
  94                   # an array containing four elements
    01                 # 1
    02                 # 2
    03                 # 3
    04                 # 4
```

In JSON this would look like this:

```JSON
[
  "hello",
  "world!",
  [
    1,
    2,
    3,
    4
  ]
]
```

You can use [msgpack-tools](https://github.com/ludocode/msgpack-tools) with the above JSON to generate `example.mp`. The below code demonstrates reading this data from a file using the Expect API:

```C
#include "mpack.h"

int main(void) {

    // Initialize a reader from a file
    mpack_reader_t reader;
    mpack_reader_init_file(&reader, "example.mp");

    // The top-level array must have exactly three elements
    mpack_expect_array_match(&reader, 3);

    // The first two elements are short strings
    char first[128];
    char second[128];
    mpack_expect_utf8_cstr(&reader, first, sizeof(first));
    mpack_expect_utf8_cstr(&reader, second, sizeof(second));

    // Next we have an array of up to ten ints
    int32_t numbers[10];
    size_t count = mpack_expect_array_max(&reader, sizeof(numbers) / sizeof(numbers[0]));
    for (size_t i = 0; i < count; ++i)
        numbers[i] = mpack_expect_i32(&reader);
    mpack_done_array(&reader);

    // Done reading the top-level array
    mpack_done_array(&reader);

    // Clean up and handle errors
    mpack_error_t error = mpack_reader_destroy(&reader);
    if (error != mpack_ok) {
        fprintf(stderr, "Error %i occurred reading data!\n", (int)error);
        return EXIT_FAILURE;
    }

    // We now know the data was parsed correctly and can safely
    // be used. The strings are null-terminated and valid UTF-8,
    // the array contained at most ten elements, and the numbers
    // are all within the range of an int32_t.
    printf("%s\n", first);
    printf("%s\n", second);
    for (size_t i = 0; i < count; ++i)
        printf("%i ", numbers[i]);
    printf("\n");

    return EXIT_SUCCESS;
}
```

With the file given above, this example will print:

```
hello
world!
1 2 3 4 
```

Note that there is only a single error check in this example. In fact each call to the reader is checking for errors and storing any error in the reader. These could be errors from reading data from the file, from invalid or corrupt MessagePack, or from not matching our expected types or ranges. On any call to the reader, if the reader was already in error or an error occurs during the call, a safe value is returned.

For example the `mpack_expect_array_max()` call above will return zero if the element is not an array, if it has more than ten elements, if the MessagePack data is corrupt, or even if the file does not exist. The `mpack_expect_utf8_cstr()` calls will also place a null-terminator at the start of the given buffer if any error occurs just in case the data is used without an error check. The error check can be performed later at a more convenient time.

## Maps

Maps can be more complicated to read because you usually want to safely handle keys being re-ordered. MessagePack itself does not specify whether maps can be re-ordered, so if you are sticking only to MessagePack implementations that preserve ordering, it may not be strictly necessary to handle this. (MPack always preserves map key ordering.) However many MessagePack implementations will ignore the order of map keys in the original data, especially in scripting languages where the data will be parsed into or encoded from an unordered map or dict. If you plan to interoperate with them, you will need to allow keys to be re-ordered.

Suppose we expect to receive a map containing two key/value pairs: a key called "compact" with a boolean value, and a key called "schema" with an int value. The example on the [MessagePack homepage](http://msgpack.org/) fits this schema, which looks like this in JSON:

```JSON
{
  "compact": true,
  "schema": 0
}
```

If we also expect the key called "compact" to always come first, then parsing this is straightforward:

```C
mpack_expect_map_match(&reader, 2);
mpack_expect_cstr_match(&reader, "compact");
bool compact = mpack_expect_bool(&reader);
mpack_expect_cstr_match(&reader, "schema");
int schema = mpack_expect_int(&reader);
mpack_done_map(&reader);
```

If we expect the "schema" key to be optional, but always after "compact", then parsing this is longer but still straightforward:

```C
size_t count = mpack_expect_map_max(&reader, 2);

mpack_expect_cstr_match(&reader, "compact");
bool compact = mpack_expect_bool(&reader);

bool has_schema = false;
int schema = -1;
if (count == 0) {
    mpack_expect_cstr_match(&reader, "schema");
    schema = mpack_expect_int(&reader);
}

mpack_done_map(&reader);
```

If however we want to allow keys to be re-ordered, then parsing this can become a lot more verbose. You need to switch on the key, but you also need to track whether each key has been used to prevent duplicate keys and ensure that required keys were found. Using the `mpack_expect_cstr()` directly for keys, this would look like this:

```C
bool has_compact = false;
bool compact = false;
bool has_schema = false;
int schema = -1;

for (size_t i = mpack_expect_map_max(&reader, 100); i > 0 && mpack_reader_error(&reader) == mpack_ok; --i) {
    char key[20];
    mpack_expect_cstr(&reader, key, sizeof(key));

    if (strcmp(key, "compact") == 0) {
        if (has_compact)
            mpack_flag_error(&reader, mpack_error_data); // duplicate key
        has_compact = true;
        compact = mpack_expect_bool(&reader);

    } else if (strcmp(key, "schema") == 0) {
        if (has_schema)
            mpack_flag_error(&reader, mpack_error_data); // duplicate key
        has_schema = true;
        schema = mpack_expect_int(&reader);

    } else {
        mpack_discard(&reader);
    }

}
mpack_done_map(&reader);

// compact is not optional
if (!has_compact)
    mpack_reader_flag_error(&reader, mpack_error_data);
```

This is obviously way too verbose. In order to simplify this code, MPack includes an Expect function called `mpack_expect_key_cstr()` to switch on string keys. This function should be passed an array of key strings and an array of bool flags storing whether each key was found. It will find the key in the given string array, check for duplicate keys, and return the index of the found key (or the key count if it is unrecognized or if an error occurs.) You would use it with an `enum` and a `switch`, like this:

```C
enum key_names       {KEY_COMPACT, KEY_SCHEMA, KEY_COUNT};
const char* keys[] = {"compact"  , "schema"  };

bool found[KEY_COUNT] = {0};
bool compact = false;
int schema = -1;

size_t i = mpack_expect_map_max(&reader, 100); // critical check!
for (; i > 0 && mpack_reader_error(&reader) == mpack_ok; --i) { // critical check!
    switch (mpack_expect_key_cstr(&reader, keys, found, KEY_COUNT)) {
        case KEY_COMPACT: compact = mpack_expect_bool(&reader); break;
        case KEY_SCHEMA:  schema  = mpack_expect_int(&reader);  break;
        default: mpack_discard(&reader); break;
    }
}

// compact is not optional
if (!found[KEY_COMPACT])
    mpack_reader_flag_error(&reader, mpack_error_data);
```

In the above examples, the call to `mpack_discard(&reader);` skips over the value for unrecognized keys, allowing the format to be extensible and providing forwards-compatibility. If you want to forbid unrecognized keys, you can flag an error (e.g. `mpack_reader_flag_error(&reader, mpack_error_data);`) instead of discarding the value.

WARNING: See above the importance of using a reasonable limit on `mpack_expect_map_max()`, and of checking for errors in each iteration of the loop. If we were to leave these out, an attacker could craft a message declaring an array of a billion elements, forcing this code into a very long loop. We specify a size of 100 here as an arbitrary limit that leaves enough space for the schema to grow in the future. If you forbid unrecognized keys, you could specify the key count as the limit.

Unlike JSON, MessagePack supports any type as a map key, so the enum integer values can themselves be used as keys. This reduces message size at some expense of debuggability (losing some of the value of a schemaless format.) There is a simpler function `mpack_expect_key_uint()` which can be used to switch on small non-negative enum values directly.

On the surface this doesn't appear much shorter than the previous code, but it becomes much nicer when you have many possible keys in a map. Of course if at all possible you should consider using the [Node API](docs/node.md) which is much less error-prone and will handle all of this for you.
