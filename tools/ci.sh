#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package, running code coverage, and building
# the Xcode project.

if [ "$(uname -s)" == "Darwin" ]; then
    unset CC
    unset CXX
    cd projects/xcode
    xcodebuild -version
    xcodebuild build -configuration Debug || exit $?
    xcodebuild build -configuration Release || exit $?
    echo "Running Debug unit tests..."
    ( cd ../.. ; projects/xcode/build/Debug/MPack ) || exit $?
    echo "Running Release unit tests..."
    ( cd ../.. ; projects/xcode/build/Release/MPack ) || exit $?
    exit 0
fi

if [[ "$AMALGAMATED" == "1" ]]; then
    tools/amalgamate.sh || exit $?
    cd build/amalgamation
fi
pwd

if [[ "$CC" == "scan-build" ]]; then
    unset CC
    unset CXX
    scan-build -o analysis --use-cc=`which clang` --status-bugs scons || exit $?

elif [[ "$CC" == "gcc" ]] && [[ "$STANDARD" == "1" ]]; then
    # We only perform code coverage measurements from the
    # GCC non-amalgamated build.
    scons gcov=1 all=1 || exit $?
    tools/gcov.sh || exit $?
    pip install --user cpp-coveralls urllib3[secure] || exit $?

    # Coveralls submission continues to experience random failures:
    #    {u'message': u"Couldn't find a repository matching this job.", u'error': True}
    # For now we'll just ignore it if it fails.
    coveralls --no-gcov --include src #|| exit $?
    true

else
    scons all=1 || exit $?

fi
