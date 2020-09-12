#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package, running code coverage, and building
# the Xcode project.

set -e

if [[ "$AMALGAMATED" == "1" ]]; then
    tools/amalgamate.sh
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    unset CC
    unset CXX
    scan-build -o analysis --use-cc=`which clang` --status-bugs tools/unittest.lua

elif [[ "$CC" == "gcov" ]]; then
    unset CC
    unset CXX
    tools/unittest.lua run-coverage
    tools/gcov.sh
    pip install --user idna==2.5 # below packages conflict with idna-2.6 (not sure if this is still necessary on bionic)
    pip install --user cpp-coveralls urllib3[secure]
    coveralls --no-gcov --include src

else
    tools/unittest.lua all

fi
