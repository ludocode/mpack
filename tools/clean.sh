#!/bin/bash
scons -c
rm -f .sconsign.dblite mpack-*.tar.gz *.gcov *.gcno src/mpack/*.o README.html
rm -f projects/vs/*.{suo,sdf,opensdf}
rm -rf build docs
rm -rf projects/vs/{Debug,Release}
