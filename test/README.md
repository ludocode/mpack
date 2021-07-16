The MPack build process does not build MPack into a library; it is used to build and run various tests. You do not need to build MPack or the unit testing suite to use MPack.

# Unit Tests

## Dependencies

The unit test suite (on Linux and Windows) requires Python 3 (at least 3.5) and Ninja (at least 1.3).

It can also make use of 32-bit cross-compiling support if on a 32-bit system. To do this, install the appropriate package:

- On Arch, this is `gcc-multilib` or `lib32-clang`
- On Ubuntu, this is `gcc-multilib` and `g++-multilib`

In addition, if Valgrind is installed and the compiler supports 32-bit cross-compiling, you will need to install Valgrind's 32-bit support, e.g. `valgrind-multilib` and/or `libc6-dbg:i386`.

### Running the unit tests

First configure the unit test suite:

```sh
tools/unit/configure.py
```

Then build and run the unit test suite with:

```
ninja -f build/unit/build.ninja
```

You can run additional tests by passing specific targets on the command-line. The "more" or "all" targets can run additional tests, and the "help" target lists tests. The CI runs "all" under various compilers.

You can change the compiler by passing a different `CC` to the configure script. For example:

```sh
CC=tcc tools/unit/configure.py
```

# Fuzz Testing

MPack supports fuzzing with american fuzzy lop. Run `tools/afl.sh` to fuzz MPack.
