#!/bin/bash
# Packages MPack up for amalgamation release. You can run tools/amalgamate.sh
# instead of this script if you just want to generate mpack.h/mpack.c.

[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }
"`dirname $0`"/clean.sh

# generate package contents
. "`dirname $0`"/amalgamate.sh
. "`dirname $0`"/gendocs.sh
cp -ar docs build/amalgamation

# create package
NAME=mpack-amalgamation-$VERSION
tar -C build/amalgamation --transform "s@^@$NAME/@" -czf build/$NAME.UNTESTED.tar.gz `ls build/amalgamation` || exit $?

# build and run all unit tests
pushd build/amalgamation
scons -j4 all=1 || exit $?
popd

# done!
mv build/$NAME.UNTESTED.tar.gz $NAME.tar.gz
echo Created $NAME.tar.gz

