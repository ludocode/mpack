# Using the Node API

The Node API is used to parse MessagePack data into a tree in memory, providing DOM-style random access to its contents.

The Node API can parse from a chunk of data in memory, or it can pull data from a file or stream. When the Node API uses a file or stream, it collects the data for a single message into one contiguous buffer in memory as it parses. The nodes for strings and binary data simply point to the data in the buffer, minimizing copies of the data. In this sense, a tree is just an index over a contiguous MessagePack message in memory.

## The Basics

A tree is initialized with one of the `init()` functions, and optionally configured (e.g. with `mpack_set_limits()`.) A complete message is then parsed with `mpack_tree_parse()`.

The data can then be accessed in random order. For example:

```C
bool parse_messagepack(const char* data, size_t count) {
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, data, count);
    mpack_tree_parse(&tree);

    mpack_node_t root = mpack_tree_root(&tree);
    do_something_with_node(root);

    return mpack_tree_destroy(&tree) == mpack_ok;
}
```

As with the Expect API, the Node API contains helper functions for extracting data of expected types from the tree, and for stepping into maps and arrays. These can be nested together to quickly unpack a message.

For example, to parse the data on the [msgpack.org](https://msgpack.org) homepage from a node:

```C
void parse_msgpack_homepage(mpack_node_t node, bool* compact, int* schema) {
    *compact = mpack_node_bool(mpack_node_map_cstr(root, "compact"));
    *schema = mpack_node_int(mpack_node_map_cstr(root, "schema"));
}
```

If any error occurs, these functions always return safe values and flag an error on the tree. You do not have to check the tree for errors in between each step; you only need an error check before using the data.

## Continuous Streams

The Node API can parse messages indefinitely from a continuous stream. This can be used for inter-process or network communications. See [msgpack-rpc](https://github.com/msgpack-rpc/msgpack-rpc) for an example networking protocol.

Here's a minimal example that wraps a tree parser around a BSD socket. We've defined a `stream_t` object to contain our file descriptor and other stream state, and we use it as the tree's context.

```C
#define MAX_SIZE (1024*1024)
#define MAX_NODES 1024

typedef struct stream_t {
    int fd;
} stream_t;

static size_t read_stream(mpack_tree_t* tree, char* buffer, size_t count) {
    stream_t* stream = tree->context;
    ssize_t step = read(stream->fd, buffer, count);
    if (step <= 0)
        mpack_tree_flag_error(tree, mpack_error_io);
}

void parse_stream(stream_t* stream) {
    mpack_tree_t tree;
    mpack_tree_init_stream(&tree, &read_stream, stream, MAX_SIZE, MAX_NODES);

    while (true) {
        mpack_tree_parse(&tree)
        if (mpack_tree_error(&tree) != mpack_error_ok))
            break;

        received_message(mpack_tree_root(&tree));
    }
}
```

The function `received_message()` will be called with each new message received from the peer.

The Node API contains many more features, including non-blocking parsing, optional map lookups and more. This document is a work in progress. See the Node API reference for more information.
