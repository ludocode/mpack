#!/bin/sh

# Builds and runs the unit test suite.
# Set CC before calling this to use a different compiler.
# Pass a configuration to run or pass "all" to run all configurations.

set -e
cd "$(dirname $0)/.."
test/unit/configure.py
ninja -f test/.build/build.ninja $@
