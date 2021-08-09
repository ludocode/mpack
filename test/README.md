The MPack build process does not build MPack into a library; it is used to build and run various tests. You do not need to build MPack or the unit testing suite to use MPack.

# Unit Tests

## Dependencies

The unit test suite (on Linux and Windows) requires Python 3 (at least 3.5) and Ninja (at least 1.3). On Windows, it also requires Visual Studio with a version of native build tools. Ninja is included in the Visual Studio 2019 build tools but not in earlier versions.

The unit test suite can also make use of 32-bit cross-compiling support if on a 32-bit Linux system. To do this, install the appropriate package:

- On Arch, this is `gcc-multilib` or `lib32-clang`
- On Ubuntu, this is `gcc-multilib` and `g++-multilib`

In addition, if Valgrind is installed and the compiler supports 32-bit cross-compiling, you will need to install Valgrind's 32-bit support, e.g. `valgrind-multilib` and/or `libc6-dbg:i386`.

## Running the unit tests

Run the build script:

```sh
tools/unit.sh
```

or on Windows:

```
tools\unit.bat
```

This will run a Python script which generates a Ninja file, then invoke Ninja on it in the default configuration.

## Running more configurations

You can run additional tests by passing specific targets on the command-line:

- The "help" target lists all targets;
- The "more" target runs additional configurations (those run by the CI);
- The "all" target runs all tests (those run on various platforms and compilers before each release);
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

The Windows script `tools\unit.bat` takes the same arguments.

### Choosing a compiler

On Linux or macOS, you can choose the compiler by passing a different `CC` to the build script. For example, to build with [TinyCC](https://bellard.org/tcc/tcc-doc.html):

```sh
CC=tcc tools/unit.sh
```

On Windows, you can run it in a build tools command prompt or load the build tools yourself to use a specific toolset. If no build tools are loaded it will load the latest Visual Studio Native Build Tools automatically.

# Fuzz Testing

MPack supports fuzzing with american fuzzy lop. Run `tools/afl.sh` to fuzz MPack.

Fuzzing paths are limited right now. The fuzzer:

 - decodes stdin with the dynamic Reader API;
 - encodes the data to a growable buffer with the Write API;
 - parses the resulting buffer with the Node API;
 - and finally, prints a debug dump of the node tree to stdout.

It thus passes all data through three major components of MPack. Not tested currently are the Expect API (and its many functions like range helpers) nor the Builder API.

# AVR / Arduino

MPack contains a Makefile for building the unit test suite for AVR. You'll need `avr-gcc` and `avr-libc` installed.

Build it like this:

```sh
make -f test/avr/Makefile
```

It doesn't actually link yet since the tests are too big. I don't have an Arduino to test it with anyway. But the object files compile at least.

# Linux Kernel

The [mpack-linux-kernel](https://github.com/ludocode/mpack-linux-kernel) project contains a KBuild and shims to build the MPack unit test suite as a Linux kernel module. This is kept separate from MPack to avoid having to dual-license MPack under the GPL.
