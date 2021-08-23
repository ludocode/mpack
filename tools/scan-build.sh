#!/bin/bash
scan-build -o analysis --status-bugs bash -c 'test/unit/configure.py && ninja -f .build/unit/build.ninja'
