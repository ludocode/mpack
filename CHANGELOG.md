Unreleased
==========

MPack develop (tentatively v0.9)
--------------------------------------------

A number of breaking API changes will be made as we approach a 1.0 release. Please take note of these changes when upgrading.

Breaking Changes:

- The mpack configuration `mpack-config.h` file is now optional, and requires `MPACK_HAS_CONFIG` in order to be included. This means you must define `MPACK_HAS_CONFIG` when upgrading or your config file will be ignored! (It is recommended to delete your config file and use the defaults.)

Changes:

- The reader's skip function is no longer ignored under `MPACK_OPTIMIZE_FOR_SIZE`.


Release Versions
================

MPack v0.8.2
------------

Changes:

- Fixed incorrect element tracking in `mpack_write_tag()`
- Added type-generic writer functions `mpack_write()` and `mpack_write_kv()`
- Added `mpack_write_object_bytes()` to insert pre-encoded MessagePack into a larger message
- Enabled strings in all builds by default
- Fixed unit test errors under `-ffast-math`
- Fixed some compiler warnings

MPack v0.8.1
------------

Changes:

- Fixed some compiler warnings
- Added various performance improvements
- Improved documentation

MPack v0.8
----------

Changes:

- Added `mpack_peek_tag()`
- Added reader helper functions to [expect re-ordered map keys](http://ludocode.github.io/mpack/md_docs_expect.html)
- [Improved documentation](http://ludocode.github.io/mpack/) and added [Pages](http://ludocode.github.io/mpack/pages.html)
- Made node key lookups check for duplicate keys
- Added various UTF-8 checking functions for reader and nodes
- Added support for compiling as C in recent versions of Visual Studio
- Removed `mpack_expect_str_alloc()` and `mpack_expect_utf8_alloc()`
- Fixed miscellaneous bugs and improved performance

MPack v0.7.1
------------

Changes:

- Removed `mpack_reader_destroy_cancel()` and `mpack_writer_destroy_cancel()`. You must now flag an error (such as `mpack_error_data`) in order to cancel reading.
- Added many code size optimizations. `MPACK_OPTIMIZE_FOR_SIZE` is no longer experimental.
- Improved and reorganized [Writer documentation](http://ludocode.github.io/mpack/group__writer.html)
- Made writer flag `mpack_error_too_big` instead of `mpack_error_io` if writing too much data without a flush callback
- Added optional `skip` callback and optimized `mpack_discard()`
- Fixed various compiler and code analysis warnings
- Optimized speed and memory usage

MPack v0.7
----------

Changes:

- Fixed various bugs in UTF-8 checking, error handler callbacks, out-of-memory and I/O errors, debug print functions and more
- Added many missing Tag and Expect functions such as `mpack_tag_ext()`, `mpack_expect_int_range()` and `mpack_expect_utf8()`
- Added extensive unit tests

MPack v0.6
----------

Changes:

- `setjmp`/`longjmp` support has been replaced by error callbacks. You can safely `longjmp` or throw C++ exceptions out of error callbacks. Be aware of local variable invalidation rules regarding `setjmp` if you use it. See the [documentation for `mpack_reader_error_t`](http://ludocode.github.io/mpack/mpack-reader_8h.html) and issue #19 for more details.
- All `inline` functions in the MPack API are no longer `static`. A single non-`inline` definition of each `inline` function is emitted, so they behave like normal functions with external linkage.
- Configuration options can now be pre-defined before including `mpack-config.h`, so you can customize MPack by defining these in your build system rather than editing the configuration file.

MPack v0.5.1
------------

Changes:

- Fixed compile errors in debug print function
- Fixed C++11 warnings

MPack v0.5
----------

Changes:

- `mpack_node_t` is now a handle, so it should be passed by value, not by pointer. Porting to the new version should be as simple as replacing `mpack_node_t*` with `mpack_node_t` in your code.
- Various other minor API changes have been made.
- Major performance improvements were made across all aspects of MPack.

MPack v0.4
----------

Changes

- Added `mpack_writer_init_growable()` to write to a growable buffer
- Converted tree parser to support node pool and pages. The Node API no longer requires an allocator.
- Added Xcode unit test project, included projects in release package
- Fixed various bugs

MPack v0.3
----------

Changes:

- Changed default config and test suite to use `DEBUG` and `_DEBUG` (instead of `NDEBUG`)
- Added Visual Studio project for running unit tests
- Fixed various bugs

MPack v0.2
----------

Changes:

- Added teardown callbacks to reader, writer and tree
- Simplified API for working with files (`mpack_file_tree_t` is now internal)

MPack v0.1
----------

Initial release.
