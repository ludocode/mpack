# Protocol Clarifications

The MessagePack specification contains overlap between different types, allowing the same data to be encoded in many different representations. For example there are overlong sequences, signed/unsigned overlap for non-negative integers, different floating-point widths, raw/str/bin/ext types, and more.

MessagePack also does not specify how types should be interpreted, such as whether maps are ordered, whether strings can be treated as binary data, whether integers can be treated as real numbers, and so on. Some of these are explicitly left up to the implementation, such as whether unrecognized extensions should be rejected or treated as opaque data.

MPack currently implements the [v5/2.0 MessagePack specification](https://github.com/msgpack/msgpack/blob/0b8f5ac67cdd130f4d4d4fe6afb839b989fdb86a/spec.md?). This document describes MPack's implementation.

## Overlong Sequences

MessagePack provides several different widths for many types. For example fixint, int8, int16, int32, int64 for integers; fixstr, str8, str16, str32 for strings; and so on. The number -20 could be encoded as `EC`, `D0 EC`, `D1 FF EC`, and more.

UTF-8 has similar overlap between codepoint widths. In UTF-8, inefficient representations of codepoints are called "overlong sequences", and decoders are required to treat them as errors. MessagePack on the other hand does not have any restrictions on inefficient representations.

- When encoding any value (other than floating point numbers), MPack always chooses the shortest representation. This is the case for integers and all compound types.

- When encoding a string, MPack always uses the str8 type if possible (which is not mapped to a "raw" from the old version of the MessagePack spec, since there was no "raw8" type.)

- When encoding an ext type, MPack always chooses a fixext type if available. This means if the ext size is 1, 2, 4, 8 or 16, the ext start tag will be encoded in two bytes (fixext and the ext type.) Otherwise MPack chooses the shortest representation.

- When decoding any value, MPack allows any length representation. Inefficiently encoded sequences are not an error. So `EC`, `D0 EC` and `D1 FF EC` would all be decoded to the same value, `mpack_type_int` of value -20.

As of this writing, all C and C++ libraries seem to write data in the shortest representation, and none forbid overlong sequences (including the reference implementation.)

## Integer Signedness

MessagePack allows non-negative integers to be encoded both as signed and unsigned, and the specification does not specify how a library should serialize them. For example the number 100 in the shortest representation could be encoded as either `CC 64` (unsigned) or `D0 64` (signed).

- When encoding, MPack writes all non-negative integers in the shortest unsigned integer type, regardless of the signedness of the input type. (The signedness of the input type is discarded.)

- When decoding as a dynamic tag or node, MPack returns the signedness of the serialized type. (This means you always need to handle both `mpack_type_int` and `mpack_type_uint`, regardless of whether you want a signed or unsigned integer, and regardless of whether you want a negative or non-negative integer.)

- When retrieving an integer with the Expect or Node APIs, MPack will automatically convert between signedness without loss of data. (For example if you call `mpack_expect_uint()` or `mpack_node_uint()`, MPack will allow both signed and unsigned data, and will flag an error if the type is signed with a negative value. Likewise if you call `mpack_expect_i8()` or `mpack_node_i8()`, MPack will allow both signed and unsigned data, and will flag an error for values below `INT8_MIN` or above `INT8_MAX`.) The expect and node integer functions allow an integer of any size or signedness, and are only checking that it falls within the range of the requested type.

A library could technically preserve the signedness of variables by writing any signed variable as int8/int16/int32/int64 or a negative fixint even if the value is non-negative. This does not seem to be the intent of the specification. For example there are no positive signed fixint values, so encoding the signed int with value 1 would take two bytes (`D0 01`) to preserve signedness. This is why MPack discards signedness.

As of this writing, all C and C++ libraries supporting the modern MessagePack specification appear to discard signedness and write all non-negative ints as unsigned (including the reference implementation.)

## Floating Point Numbers

In addition to the integer types, the MessagePack specification includes "float 32" and "float 64" types for real numbers.

- When encoding, MPack writes real numbers as the original width of the data (so `mpack_write_float()` writes a "float 32", and `mpack_write_double()` writes a "float 64".)

- When decoding as a dynamic tag or node, MPack returns the width of the serialized type. (It is recommended to handle both `mpack_type_float` and `mpack_type_double` (or neither) since other libraries may write real numbers in any width.)

- When expecting a real number with the Expect API, or when getting a float or double from a node in the Node API, MPack includes two different sets of functions:

  - The lax versions are the default. These will allow any integer or real type and convert it to the expected type, which may involve loss of precision. These include `mpack_expect_float()`, `mpack_expect_double()`, `mpack_node_float()` and `mpack_node_double()`.

  - The strict versions, suffixed by `_strict`, will allow only real numbers, and only of a width of at least that of the expected type. So `mpack_node_float_strict()` or `mpack_expect_float_strict()` allow only "float 32", while `mpack_node_double_strict()` or `mpack_expect_double_strict()` allow both "float 32" and "float 64".

    - If you want to allow only a "float 64", you would have to read a tag or check the node type and make sure it contains `mpack_type_double`.

    - If you want a `float` version that allows either "float 32" or "float 64" but not integers, you could use `(float)mpack_node_double_strict()` or `(float)mpack_expect_double_strict()`. But if you are using `float` you probably don't care much about precision anyway so you should just use `mpack_node_float()` or `mpack_expect_float()`.

MessagePack libraries in dynamic languages may support an option to generate floats instead of doubles for space efficiency. If you're converting data from JSON, you could use [json2msgpack -f](https://github.com/ludocode/msgpack-tools) to convert to floats instead of doubles.

## Map Ordering

MessagePack does not specify the ordering of map key/value pairs. Key/value pairs have a well-defined order when serialized, but the specification does not specify whether implementations should observe it when encoding or preserve it when decoding, and does not specify whether it should be adapted to an ordered associative array when de-serialized.

MPack always preserves map ordering. Key/value pairs are written in the given order in the write API, read in the serialized order in the read and expect APIs, and provided in their original serialized order in the Node API. In particular this means `mpack_node_map_key_at()` and `mpack_node_map_value_at()` are always ordered as stored in the original serialized data. An application using only MPack can always assume a fixed map order.

However, MPack strongly recommends writing code that allows for map re-ordering. This is for two reasons:

- MessagePack is often used to interface with languages that do not preserve map ordering. For example the msgpack-python library in Python unpacks a map to a `dict`, not an `OrderedDict`. Many languages use hashtables to store keys, so MessagePack encoded by these languages will have map key/value pairs in a random order. The order may be different between compiler or interpreter versions even for identical map content.

- MessagePack is designed to be at least partly compatible with JSON. It is sometimes converted from JSON, and is sometimes recommended as an efficient replacement for JSON. Unlike MessagePack, JSON explicitly allows map re-ordering. Two JSON documents that have re-ordered key/value pairs but are otherwise the same are considered equivalent.

MPack contains functions to make it easy to parse messages with re-ordered map pairs. For the Node API, lookup functions such as `mpack_node_map_cstr()` and `mpack_node_map_int()` will find the value for a given key regardless of ordering. For the expect API, the functions `mpack_expect_key_cstr()` and `mpack_expect_key_uint()` can be used to switch on a key in a read loop, which allows parsing map pairs in any order.

## Duplicate Map Keys

MessagePack has no restrictions against duplicate keys in a map, so MPack allows duplicate keys in maps. Iterating over a map in the Node or Reader will provide key/value pairs in serialized order and will not flag any errors for duplicates. However, helper functions that compare keys (such as the "lookup" or "match" functions) do check for duplicates.

In the Node API, the MPack lookup functions that search for a given key to find its value always check for duplicates. They are meant to provide a unique value for a given key. For example `mpack_node_map_cstr()` and `mpack_node_map_int()` will always check the whole map and will flag an error if a duplicate key is found. If you want to find multiple values for a given key, you will need to iterate over them manually with `mpack_node_map_key_at()` and `mpack_node_map_value_at()`.

In the Expect API, the key match functions (such as `mpack_expect_key_cstr()` and `mpack_expect_key_uint()`) check for duplicate keys, and will flag an error when a duplicate is found. If you want to use the match functions with duplicates, you can toggle off the `bool` flag corresponding to a found key to allow it to be matched again. This allows implicit checking of duplicate keys, with an opt-in to safely handle duplicates in order.

Despite the allowance for duplicate keys, MPack recommends against providing multiple values for the same key in order to more safely interface with other languages and formats (as with the Map Ordering recommendations above.) A safer and more explicit way to accomplish this is to simply use an array containing the desired values as the single value for a map key.

## v4 Compatibility

The MessagePack [v4/1.0 spec](https://github.com/msgpack/msgpack/blob/acbcdf6b2a5a62666987c041124a10c69124be0d/spec-old.md?) did not distinguish between strings and binary data. It only provided the "raw" type in widths of fixraw, raw16 and raw32, which was used for both. The [v5/2.0 spec](https://github.com/msgpack/msgpack/blob/0b8f5ac67cdd130f4d4d4fe6afb839b989fdb86a/spec.md?) on the other hand renames the raw type to str, adds the bin type to represent binary data, and adds an 8-bit width for strings called str8. This means that even when binary data is not used, the new specification is not backwards compatible with the old one that expects raw to contain strings, because a modern encoder will use the str8 type. The new specification also adds an ext type to distinguish between arbitrary binary blobs and MessagePack extensions.

- MPack by default always encodes with the str8 type for strings where possible. This means that MessagePack encoded with MPack is by default not backwards compatible with decoders that only understand the raw types from the old specification. This matches the behaviour of other C/C++ libraries that support the modern spec, including the reference implementation.

- Since MPack allows overlong sequences, it does not require that the str8 type be used, so data encoded with an old-style encoder will be parsed correctly by MPack (with raw types parsed as strings.)

MPack supports a compatibility mode if interoperability is required with applications or data that do not support the new (v5) MessagePack spec. To use it, you must define `MPACK_COMPATIBILITY`, and then call `mpack_writer_set_version()` with the value `mpack_version_v4`. A writer in this mode will never use the str8 type, and will output the old fixraw, raw16 and raw32 types when writing both strings and bins.

Note that there is no mode to configure readers to interpret the old style raws as either bins or strings. They will only be interpreted as strings.

## Extension Types

The MessagePack specification defines support for extension types. These are like bin types, but with an additional marker to define the semantic type, giving a clue to parsers about how to interpret the contained bits.

This author considers them redundant with the bin type. Adding semantic information about binary data does nothing to improve interoperability. It instead only increases the complexity of decoders and fragments support for MessagePack. In my opinion the MessagePack spec should be forever frozen at v5 without extension types, similar (in theory) to JSON. See the discussion in [msgpack/msgpack!206](https://github.com/msgpack/msgpack/issues/206#issuecomment-386066548) for more.

MPack discourages the use of extension types. In its default configuration, MPack will flag `mpack_error_unsupported` when encountering them, and functions to encode them are preproc'd out. Support for extension types can be enabled by defining `MPACK_EXTENSIONS`.
