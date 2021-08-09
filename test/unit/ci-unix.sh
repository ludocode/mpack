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
    ninja -f .build/unit/build.ninja
    exit $?
fi

# Run the "more" variant of unit tests
tools/unit.sh more

if [ "$CC" = "gcc" ]; then
    # Collect coverage info.
    # This is submitted to Coveralls as a separate GitHub Action.
    unset CC
    unset CXX
    tools/coverage.sh
fi
