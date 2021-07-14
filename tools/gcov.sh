#!/bin/sh
# The "run-coverage" target runs the unit tests with . It's not included in "all".
tools/unit.sh run-coverage
gcov --object-directory test/.build/coverage/objs/src/mpack `find src -name '*.c'` || exit $?
