#!/bin/bash
scan-build -o analysis --use-cc=`which clang` --status-bugs --view scons
