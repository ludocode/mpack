# Feature Comparisons

This document compares the features of various C libraries for encoding and decoding MessagePack. (It attempts to be neutral, but you may find it biased towards MPack since it is part of this project.)

Feedback is welcome! Please let me know if any entries in the table are incorrect, or if there are any killer features from other parsers that are missing in this list.

## Feature Matrix

[mpack]: https://github.com/ludocode/mpack
[msgpack-c]: https://github.com/msgpack/msgpack-c
[cmp]: https://github.com/camgunz/cmp
[cwpack]: https://github.com/clwi/CWPack

|    | [MPack][mpack]<br>(v1.1 beta) | [msgpack-c][msgpack-c]<br>(v3.1.1) | [CMP][cmp]<br>(v18) | [CWPack][cwpack]<br>(v1.1) |
|:------------------------------------|:---:|:---:|:---:|:---:|
| No libc requirement                 | ✓   |     | ✓   | ✓   |
| No allocator requirement            | ✓   |     | ✓   | ✓   |
| Growable memory writer              | ✓   | ✓   |     | ✓\* |
| File I/O helpers                    | ✓   | ✓   |     | ✓\* |
| Tree parser                         | ✓   | ✓   |     |     |
| Propagating errors                  | ✓   |     | ✓   |     |
| Descriptive error information       |     |     |     |     |
| Compound size tracking              | ✓   |     |     |     |
| Automatic compound size             | ✓   |     |     |     |
| Incremental parser                  | ✓   |     | ✓   | ✓   |
| Typed read helpers                  | ✓   |     | ✓   |     |
| Range/match read helpers            | ✓   |     |     |     |
| Asynchronous incremental parser     |     |     |     |     |
| Peek next element                   | ✓   |     |     |     |
| Tree stream parser                  | ✓   | ✓   |     |     |
| Asynchronous tree stream parser     | ✓   | ✓   |     |     |
| Support for new (2.0) spec          | ✓   | ✓   | ✓   | ✓   |
| Compatible with older (1.0) spec    | ✓   | ✓   | ✓   | ✓   |
| UTF-8 verification                  | ✓   |     |     |     |
| Type-generic write helpers          | ✓   | ✓   |     |     |
| Timestamps                          | ✓   | ✓   |     |     |

Most of the features above are optional when supported and can be configured in all libraries. In particular, UTF-8 verification is optional with MPack; compound size tracking is optional and disabled in release by default with MPack; and 1.0 (v4) spec compatibility is optional in all libraries (v5/2.0 is the recommended and default usage.)

\*CWPack's [goodies](https://github.com/clwi/CWPack/tree/master/goodies) are included in the above table.

## Glossary

*Tree parsing* means parsing a MessagePack object into a DOM-style tree of dynamically-typed elements supporting random access.

*Incremental parsing* means being able to parse one basic MessagePack element at a time (either imperatively or with a SAX-style callback) with no per-element memory usage.

*Propagating errors* means a parse error or type error on one element places the whole parser, encoder or tree in an error state. This means you can check for errors only at certain key points rather than at every interaction with the library, and you get a final error state indicating whether any error occurred at any point during parsing or encoding.

*Descriptive error information* means being able to get additional information when an error occurs, such as the tree position and byte position in the message where the error occurred.

*Compound size tracking* means verifying that the same number of child elements or bytes were written or read as was declared at the start of a compound element.

*Automatic compound size* means not having to specify the number of elements or bytes in an element up-front, instead determining it automatically when the element is closed.

*Typed read helpers* means helper functions for a parser that can check the expected type of an element and return its value in that type. For example `cmp_read_int()` in CMP or `mpack_expect_u32()` in MPack.

*Range/match read helpers* means helper functions for a parser that can check not only the expected type of an element, but also enforce an allowed range or exact match on a given value.

*Peeking the next element* means being able to view the next element during incremental parsing without popping it from the data buffer.

*Tree stream parsing* means the ability to parse a continuous stream of MessagePack with no external delimitation, emitting complete objects with a DOM-style tree API as they are found. The parser must be able to pause if the data is incomplete and continue parsing later when more data is available. This is necessary for implementing RPC over a socket with a tree parser (without needing to delimit messages by size.)

*Asynchronous* means the parser is cooperative and re-entrant; when not enough data is available, it will return out of the parser with a "continue later" result rather than failing or blocking the read. There are two asynchronous entries in the above table, one for incremental parsing and one for tree parsing.

*Compatible with older (1.0) spec* means the ability to produce messages compatible with parsers that only understand the old v4/1.0 version of MessagePack. A backwards-compatible encoder must at a minimum support writing an old-style raw without "str8", since there was no "raw8" type in old MessagePack.

*Type-generic write helpers* means a generic write function or macro that can serialize based on the static type of the argument, in at least one of C11 or C++. (The reference [msgpack-c][msgpack-c] currently supports this only in C++ mode. MPack supports this both in C++ with templates and in C11 with `_Generic`.)
