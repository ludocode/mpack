
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
| DOM-style parser                    | ✓   | ✓   |     |
| Propagating errors                  | ✓   |     | ✓   |
| Compound size tracking              | ✓   |     |     |
| Incremental parser                  | ✓   |     | ✓   |
| Incremental type helpers            | ✓   |     | ✓   |
| Incremental range/match helpers     | ✓   |     |     |
| Asynchronous incremental parser     |     |     |     |
| Peek next element                   | ✓   |     |     |
| DOM stream parser                   |     | ✓   |     |
| Asynchronous DOM stream parser      |     | ✓   |     |
| Compatible with older (1.0) spec    |     |     | ✓   |
| UTF-8 verification                  | ✓   |     |     |

All features in the above list are optional for all libraries, aside from msgpack-c's DOM-style parser since it has no incremental parser. In particular, UTF-8 verification is optional with MPack; compound size tracking is optional and disabled in release by default with MPack; and 1.0 (v4) spec compatibility is optional with CMP (2.0/v5 is the recommended and default usage.)

## Glossary

*DOM-style parsing* means parsing a MessagePack object into a tree of dynamically-typed elements supporting random access.

*Propagating errors* means a parse error or type error on one element places the whole parser, encoder or DOM tree in an error state. This means you can check for errors only at certain key points rather than at every interaction with the library, and you get a final error state indicating whether any error occurred at any point during parsing or encoding.

*Compound size tracking* means verifying that the same number of child elements or bytes were written or read as was declared at the start of a compound element.

*Incremental parsing* means being able to parse one basic MessagePack element at a time (either imperatively or with a SAX-style callback) with no per-element memory usage.

*Incremental type helpers* means helper functions for an incremental parser that can check the expected type of an element.

*Incremental range/match helpers* means helper functions for an incremental parser that can check not only the expected type of an element, but also enforce an allowed range or exact match on a given value.

*Peeking the next element* means being able to view the next element during incremental parsing without popping it from the data buffer.

*DOM Stream parsing* means the ability to parse a continuous stream of MessagePack with no external delimitation, emitting complete objects with a DOM-style API as they are found. The parser must be able to pause if the data is incomplete and continue parsing later when more data is available.

*Asynchronous* means the parser is cooperative and re-entrant; when not enough data is available, it will return out of the parser with a "continue later" result rather than failing or blocking the read. There are two asynchronous entries in the above table, one for incremental parsing and one for DOM-style parsing.

