The MPack build process does not build MPack into a library; it is used to build and run various tests. You do not need to build MPack or the unit testing suite to use MPack.

# Unit Tests

## Linux

### Dependencies

The unit test suite on Linux has the following requirements:

- Lua, at least version 5.1, with the following rocks:
    - luafilesystem
    - rapidjson (this requires CMake to build)
- Ninja
- 32-bit cross-compiling support, if on a 64-bit system
    - On Arch, this is `gcc-multilib` or `lib32-clang`
    - On Ubuntu, this is `gcc-multilib` and `g++-multilib`
- Valgrind
    - Including debugging 32-bit apps with 64-bit Valgrind, e.g. `valgrind-multilib`, `libc6-dbg:i386`

For example, on Ubuntu:

```sh
# This installs Lua 5.1. You may need to change it for newer versions of Ubuntu.
sudo apt install build-essential gcc-multilib g++-multilib cmake lua5.1 luarocks ninja-build libc6-dbg:i386 valgrind
```

Or on Arch:

```sh
sudo pacman -S --needed gcc-multilib cmake lua luarocks ninja valgrind
```

Then:

```sh
sudo luarocks install luafilesystem
sudo luarocks install rapidjson
```

### Running the tests

Run the default tests with: `tools/unittest.lua`

You can run additional tests by passing specific targets on the command-line. The "more" or "all" targets can run additional tests, and the "help" target lists tests. The CI runs "all" under various compilers.

## Other platforms

On Windows, there is a Visual Studio solution, and on OS X, there is an Xcode project for building and running the test suite.

# Fuzz Testing

MPack supports fuzzing with american fuzzy lop. Run `tools/afl.sh` to fuzz MPack.
