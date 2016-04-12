#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build", which runs the full debug build
# under scan-build; otherwise all variants are built. Also handles
# building the amalgamated package and running code coverage.

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
    coveralls --no-gcov --include src || exit $?

    # Note to self: if the coveralls submission is failing with:
    #    {u'message': u"Couldn't find a repository matching this job.", u'error': True}
    # Try logging in/out of coveralls to refresh OAuth token.
    #    https://github.com/lemurheavy/coveralls-public/issues/632#issuecomment-148815369

else
    scons all=1 || exit $?

fi
