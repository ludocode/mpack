#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package.

if [[ "$AMALGAMATED" == "1" ]]; then
    tools/package.sh || exit $?
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    export CC=`which clang`
    scan-build --use-cc="$CC" --status-bugs scons
else
    scons all=1
fi
