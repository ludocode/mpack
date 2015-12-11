#!/bin/bash
scons -c
rm -rf .sconsign.dblite .sconf_temp config.log
rm -f mpack-*.tar.gz *.gcov *.gcno src/mpack/*.o README.html
rm -f projects/vs/*.{suo,sdf,opensdf}
rm -rf docs/*.{png,html,css,js} docs/search
rm -rf build analysis
rm -rf projects/vs/{Debug,Release}
