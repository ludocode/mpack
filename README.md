
## Introduction

MPack is a C implementation of an encoder and decoder for the [MessagePack](http://msgpack.org/) serialization format. It is intended to be:

 * Simple and easy to use
 * Secure against untrusted data
 * Lightweight, suitable for embedded
 * Helpful for debugging
 * [Extensively documented](http://ludocode.github.io/mpack/)

The core of MPack contains a buffered reader and writer with a custom callback to fill or flush the buffer. Helper functions can be enabled to read values of expected type, to work with files, to allocate strings automatically, to check UTF-8 encoding, and more. The MPack featureset can be configured at compile-time to set which features, components and debug checks are compiled, and what dependencies are available.

The MPack code is small enough to be embedded directly into your codebase. The easiest way to use it is to download the [amalgamation package](https://github.com/ludocode/mpack/releases) and insert the source files directly into your project. Copy `mpack.h` and `mpack.c` into to your codebase, and copy `mpack-config.h.sample` as `mpack-config.h`. You can use the defaults or edit it if you'd like to customize the MPack featureset.

MPack is written in the portable intersection of C99 and C++. In other words, it's written in C99, but if you are stuck using a certain popular compiler from a certain unpopular vendor that refuses to support C99, you can compile it as C++ instead. (The headers should also be C89-clean, provided compatible definitions of `bool`, `inline`, `stdint.h` and friends are available, and `const` is defined away.)

*NOTE: MPack is beta software under development. There are still some TODOs in the codebase, some security issues to fix, some MessagePack 1.0/1.1 compatibility and interoperability issues to sort out, some test suite portability issues to fix, and there is only around 45% unit test coverage.*

## The Node Reader API

The Node API parses a chunk of MessagePack data into an immutable tree of dynamically-typed nodes. A series of helper functions can be used to extract data of specific types from each node.

    // parse a file into a node tree
    mpack_tree_t tree;
    mpack_tree_init_file(&tree, "homepage-example.mp", 0);
    mpack_node_t* root = mpack_tree_root(&tree);

    // extract the example data on the msgpack homepage
    bool compact = mpack_node_bool(mpack_node_map_cstr(root, "compact"));
    int schema = mpack_node_i32(mpack_node_map_cstr(root, "schema"));

    // clean up and check for errors
    if (mpack_tree_destroy(tree) != mpack_ok) {
        fprintf(stderr, "An error occurred decoding the data!\n");
        return;
    }

Note that no additional error handling is needed in the above code. If the file is missing or corrupt, if map keys are missing or if nodes are not in the expected types, special "nil" nodes and false/zero values are returned and the tree is placed in an error state. An error check is only needed before using the data. Alternatively, the tree can be configured to longjmp in such cases if a handler is set.

## The Static Write API

The MPack Write API encodes structured data of a fixed (hardcoded) schema to MessagePack.

    // encode to memory buffer
    char buffer[256];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, sizeof(buffer));

    // write the example on the msgpack homepage
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "compact");
    mpack_write_bool(&writer, true);
    mpack_write_cstr(&writer, "schema");
    mpack_write_uint(&writer, 0);
    mpack_finish_map(&writer);

    // clean up
    size_t count = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        fprintf(stderr, "An error occurred encoding the data!\n");
        return;
    }

In the above example, we encode only to an in-memory buffer. The writer can optionally be provided with a flush function (such as a file or socket write function) to call when the buffer is full or when writing is done.

If any error occurs, the writer is placed in an error state and can optionally longjmp if a handler is set. The writer will flag an error if too much data is written, if the wrong number of elements are written, if the data could not be flushed, etc.

The MPack writer API is imperative in nature. Since it's easy to incorrectly compose structured data with an imperative API, MPack provides a debug mode which tracks reads and writes of compound types to ensure that the correct number of elements and bytes were read and written. This allows for rapid development of high-performance applications that use MessagePack. In release mode, these checks are eliminated for maximum performance.

## Why Not Just Use JSON?

Conceptually, MessagePack stores data similarly to JSON: they are both composed of simple values such as numbers and strings, stored hierarchically in maps and arrays. So why not just use JSON instead? The main reason is that JSON is designed to be human-readable, so it is not as efficient as a binary serialization format:

- Compound types such as strings, maps and arrays are delimited, so appropriate storage cannot be allocated upfront. The whole object must be parsed to determine its size.

- Strings are not stored in their native encoding. They cannot contain quotes or special characters, so they must be escaped when written and converted back when read.

- Numbers are particularly inefficient (especially when parsing back floats), making JSON inappropriate as a base format for structured data that contains lots of numbers.

The above issues greatly increase the complexity of the decoder. Full-featured JSON decoders are quite large, and minimal decoders tend to leave out such features as string unescaping and float parsing, instead leaving these up to the user or platform. This can lead to hard-to-find and/or platform-specific bugs. This also significantly decreases performance, making JSON unattractive for use in applications such as mobile games.

While the space inefficiencies of JSON can be partially mitigated through minification and compression, the performance inefficiencies cannot. More importantly, if you are minifying and compressing the data, then why use a human-readable format in the first place?

## Running the Unit Tests

The MPack build process does not build MPack into a library; it is used to build and run the unit tests. You do not need to build MPack or the unit testing suite to use MPack. The test suite currently only supports Linux.

The test suite uses SCons and requires Valgrind, and can be run in the repository or in the amalgamation package. SCons will build and run the library and unit testing suite in both debug and release for both 32-bit and 64-bit architectures (if available), so it will take a minute or two to build and run. If you are on 64-bit, you will also need support for running 32-bit binaries with 64-bit Valgrind (`libc6-dbg:i386` on Ubuntu or `valgrind-multilib` on Arch.) Just run `scons` to build and run all tests.

