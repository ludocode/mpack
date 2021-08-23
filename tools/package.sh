#!/bin/bash
# Packages MPack up for amalgamation release. You can run tools/amalgamate.sh
# instead of this script if you just want to generate mpack.h and mpack.c.

set -v
set -e

[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }
"`dirname $0`"/clean.sh

# generate package contents
. "`dirname $0`"/amalgamate.sh
. "`dirname $0`"/gendocs.sh
cp -ar .build/docs/html .build/amalgamation/docs
sed -i '/#define MPACK_AMALGAMATED 1/a\
#define MPACK_RELEASE_VERSION 1' .build/amalgamation/src/mpack/mpack.h

# create package
NAME=mpack-amalgamation-$VERSION
tar -C .build/amalgamation --transform "s@^@$NAME/@" -czf .build/$NAME.UNTESTED.tar.gz `ls .build/amalgamation` || exit $?

# build and run all unit tests
pushd .build/amalgamation
tools/unit.sh all || exit $?
popd

# done!
mv .build/$NAME.UNTESTED.tar.gz $NAME.tar.gz
echo Created $NAME.tar.gz
