The MPack build process does not build MPack into a library; it is used to build and run various tests. You do not need to build MPack or the unit testing suite to use MPack.

# Unit Tests

## Dependencies

The unit test suite (on Linux and Windows) requires Python 3 (at least 3.5) and Ninja (at least 1.3).

It can also make use of 32-bit cross-compiling support if on a 32-bit system. To do this, install the appropriate package:

- On Arch, this is `gcc-multilib` or `lib32-clang`
- On Ubuntu, this is `gcc-multilib` and `g++-multilib`

In addition, if Valgrind is installed and the compiler supports 32-bit cross-compiling, you will need to install Valgrind's 32-bit support, e.g. `valgrind-multilib` and/or `libc6-dbg:i386`.

### Running the unit tests

Run the build script:

```sh
tools/unit.sh
```

This will run a Python script which generates a Ninja file, then invoke Ninja on it in the default configuration.

You can run additional tests by passing specific targets on the command-line:

- The "help" target lists all targets;
- The "more" target runs additional configurations (those run by the CI);
- The "all" target runs all tests (those run on various platforms and compilers before each release,);
- Most targets take the form `run-{name}-{mode}` where `{name}` is a configuration name and `{mode}` is either `debug` or `release`.

For example, to run all tests with the default compiler:

```sh
tools/unit.sh all
```

To run only a specific configuration, say, the debug `noio` configuration where `MPACK_STDIO` is disabled:

```sh
tools/unit.sh run-noio-debug
```

To list all targets:

```sh
tools/unit.sh help
```

You can also change the compiler by passing a different `CC` to the configure script. For example, to build with [TinyCC](https://bellard.org/tcc/tcc-doc.html):

```sh
CC=tcc tools/unit.sh
```

# Fuzz Testing

MPack supports fuzzing with american fuzzy lop. Run `tools/afl.sh` to fuzz MPack.
