#!/bin/sh

# Builds and runs the unit test suite under UNIX.
#
# Pass environment variable CC to specify the compiler.
#
# This script is run by the continuous integration server to test MPack on UNIX
# systems.

set -e

if [ "$AMALGAMATED" = "1" ]; then
    tools/amalgamate.sh
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    unset CC
    unset CXX

    echo "Not yet implemented!"
    exit 1

    scan-build -o analysis --use-cc=`which clang` --status-bugs test/unit/configure.py
    ninja -f build/unit/build.ninja
    exit $?
fi

if [ "$CC" = "gcov" ]; then
    unset CC
    unset CXX
    tools/gcov.sh
    pip install --user idna==2.5 # below packages conflict with idna-2.6 (not sure if this is still necessary on bionic)
    pip install --user cpp-coveralls urllib3[secure]
    coveralls --no-gcov --include src
    exit $?
fi

# Run the "more" variant of unit tests
tools/unit.sh more
