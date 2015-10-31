#!/bin/sh
# tests must be built with "scons gcov=1"
gcov --object-directory build/debug/src/mpack `find src -name '*.c'` || exit $?
