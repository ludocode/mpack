#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package and running code coverage.

if [[ "$AMALGAMATED" == "1" ]]; then
    tools/package.sh || exit $?
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    export CC=`which clang`
    scan-build --use-cc="$CC" --status-bugs scons
elif [[ "$CC" == "gcc" ]] && [[ "$STANDARD" == "1" ]]; then
    # We only perform code coverage measurements from the
    # GCC non-amalgamated build.
    scons gcov=1 all=1 || exit $?
    tools/gcov.sh || exit $?
    coveralls --no-gcov --include src || exit $?
else
    scons all=1 || exit $?
fi
