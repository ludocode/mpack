#!/bin/sh
set -e

# run unit tests with coverage
tools/unit.sh run-coverage

# run gcov for traditional text-based coverage output
rm -rf coverage
mkdir -p coverage
gcov --object-directory .build/unit/coverage/objs/src/mpack `find src -name '*.c'` || exit $?
mv *.gcov coverage

# run lcov
lcov --capture --directory .build/unit/coverage/objs/src --output-file coverage/lcov.info

# generate HTML coverage
genhtml coverage/lcov.info --output-directory coverage/html

echo
echo "Done. Results written in coverage/"
echo "View HTML results in: file://$(pwd)/coverage/html/index.html"
