#!/bin/bash
# used to run the test suite under continuous integration. handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built.
if [[ "$CC" == "scan-build" ]]; then
    export CC=`which clang`
    scan-build --use-cc="$CC" --status-bugs scons
else
    scons all=1
fi
