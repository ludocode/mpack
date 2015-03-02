#!/bin/bash
scons -c
rm -rf build docs .sconsign.dblite mpack-*.tar.gz *.gcov src/mpack/*.o
