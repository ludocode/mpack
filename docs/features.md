
# Feature Comparisons

This document compares the features of various C libraries for encoding and decoding MessagePack. (It attempts to be neutral, but you may find it biased towards MPack since it is part of this project.)

Feedback is welcome! Please let me know if any entries in the table are incorrect, or if there are any killer features from other parsers that are missing in this list.

## Feature Matrix

| | [MPack](https://github.com/ludocode/mpack) (v0.8) | [msgpack-c](https://github.com/msgpack/msgpack-c) (v1.3.0) | [CMP](https://github.com/camgunz/cmp) (v14) |
|:------------------------------------|:---:|:---:|:---:|
| No libc requirement                 | ✓   |     | ✓   |
| No allocator requirement            | ✓   |     | ✓   |
| Growable memory writer              | ✓   | ✓   |     |
| File I/O helpers                    | ✓   | ✓   |     |
| Tree parser                         | ✓   | ✓   |     |
| Propagating errors                  | ✓   |     | ✓   |
| Compound size tracking              | ✓   |     |     |
| Automatic compound size             |     |     |     |
| Incremental parser                  | ✓   |     | ✓   |
| Incremental type helpers            | ✓   |     | ✓   |
| Incremental range/match helpers     | ✓   |     |     |
| Asynchronous incremental parser     |     |     |     |
| Peek next element                   | ✓   |     |     |
| Tree stream parser                  |     | ✓   |     |
| Asynchronous tree stream parser     |     | ✓   |     |
| Support for new (2.0) spec          | ✓   | ✓   | ✓   |
| Compatible with older (1.0) spec    |     |     | ✓   |
| UTF-8 verification                  | ✓   |     |     |

Most of the features above are optional and can be configured in all libraries. In particular, UTF-8 verification is optional with MPack; compound size tracking is optional and disabled in release by default with MPack; and 1.0 (v4) spec compatibility is optional with CMP (v5/2.0 is the recommended and default usage.)

(The goal of MPack for a 1.0 release is to support everything in this list.)

## Glossary

*Tree parsing* means parsing a MessagePack object into a DOM-style tree of dynamically-typed elements supporting random access.

*Propagating errors* means a parse error or type error on one element places the whole parser, encoder or tree in an error state. This means you can check for errors only at certain key points rather than at every interaction with the library, and you get a final error state indicating whether any error occurred at any point during parsing or encoding.

*Compound size tracking* means verifying that the same number of child elements or bytes were written or read as was declared at the start of a compound element.

*Automatic compound size* means not having to specify the number of elements or bytes in an element up-front, instead determining it automatically when the element is closed.

*Incremental parsing* means being able to parse one basic MessagePack element at a time (either imperatively or with a SAX-style callback) with no per-element memory usage.

*Incremental type helpers* means helper functions for an incremental parser that can check the expected type of an element.

*Incremental range/match helpers* means helper functions for an incremental parser that can check not only the expected type of an element, but also enforce an allowed range or exact match on a given value.

*Peeking the next element* means being able to view the next element during incremental parsing without popping it from the data buffer.

*Tree stream parsing* means the ability to parse a continuous stream of MessagePack with no external delimitation, emitting complete objects with a DOM-style tree API as they are found. The parser must be able to pause if the data is incomplete and continue parsing later when more data is available. This is necessary for implementing RPC over a socket with a tree parser (without needing to delimit messages by size.)

*Asynchronous* means the parser is cooperative and re-entrant; when not enough data is available, it will return out of the parser with a "continue later" result rather than failing or blocking the read. There are two asynchronous entries in the above table, one for incremental parsing and one for tree parsing.

*Compatible with older (1.0) spec* means the ability to produce messages compatible with parsers that only understand the old v4/1.0 version of MessagePack. A backwards-compatible encoder must at a minimum support the option to disable "str8", since there was no "raw8" type in old MessagePack.

