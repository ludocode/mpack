#!/bin/sh
# tests must be run with "test/build.lua run-coverage". it's not included in "all".
gcov --object-directory build/coverage/objs/src/mpack `find src -name '*.c'` || exit $?
