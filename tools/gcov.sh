#!/bin/sh
build/debug/mpack-test || exit $?
gcov --object-directory build/debug/src/mpack `find src -name '*.c'` || exit $?
