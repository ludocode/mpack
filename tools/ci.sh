#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package, running code coverage, and building
# the Xcode project.

set -e

if [ "$(uname -s)" == "Darwin" ]; then
    unset CC
    unset CXX
    cd projects/xcode
    xcodebuild -version
    xcodebuild build -configuration Debug
    xcodebuild build -configuration Release
    echo "Running Debug unit tests..."
    ( cd ../.. ; projects/xcode/build/Debug/MPack )
    echo "Running Release unit tests..."
    ( cd ../.. ; projects/xcode/build/Release/MPack )
    exit 0
fi

if [[ "$AMALGAMATED" == "1" ]]; then
    tools/amalgamate.sh
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    unset CC
    unset CXX
    scan-build -o analysis --use-cc=`which clang` --status-bugs scons

elif [[ "$CC" == "gcov" ]]; then
    unset CC
    unset CXX
    scons gcov=1
    tools/gcov.sh
    pip install --user cpp-coveralls urllib3[secure]
    coveralls --no-gcov --include src

else
    scons all=1

fi
