#!/bin/bash
# packages MPack up for amalgamation release

"`dirname $0`"/clean.sh

# generate docs (on non-ci builds)
if [[ "$CI" == "" ]]; then
    . "`dirname $0`"/gendocs.sh || exit $?
else
    mkdir -p docs
fi

FILES="\
    mpack-platform \
    mpack-common \
    mpack-writer \
    mpack-reader \
    mpack-expect \
    mpack-node"

# add top license and comment
rm -rf build/amalgamation
mkdir -p build/amalgamation/src/mpack
HEADER=build/amalgamation/src/mpack/mpack.h
SOURCE=build/amalgamation/src/mpack/mpack.c
echo '/**' > $HEADER
sed 's/^/ * /' LICENSE >> $HEADER
cat - >> $HEADER <<EOF
 */

/*
 * This is the MPack $VERSION amalgamation package.
 *
 * http://github.com/ludocode/mpack
 */

EOF
cp $HEADER $SOURCE

# assemble header
echo -e "#ifndef MPACK_H\n#define MPACK_H 1\n" >> $HEADER
echo -e "#define MPACK_AMALGAMATED 1\n" >> $HEADER
echo -e "#include \"mpack-config.h\"\n" >> $HEADER
for f in $FILES; do
    echo -e "\n/* $f.h */" >> $HEADER
    sed -e 's@^#include ".*@/* & */@' -e '0,/^ \*\/$/d' src/mpack/$f.h >> $HEADER
done
echo -e "#endif\n" >> $HEADER

# assemble source
echo -e "#define MPACK_INTERNAL 1" >> $SOURCE
echo -e "#define MPACK_EMIT_INLINE_DEFS 1\n" >> $SOURCE
echo -e "#include \"mpack.h\"\n" >> $SOURCE
for f in $FILES; do
    echo -e "\n/* $f.c */" >> $SOURCE
    sed -e 's@^#include ".*@/* & */@' -e '0,/^ \*\/$/d' src/mpack/$f.c >> $SOURCE
done

# assemble package contents
CONTENTS="docs test SConscript SConstruct LICENSE README.md"
cp -ar $CONTENTS build/amalgamation
mkdir -p build/amalgamation/projects/{vs,xcode/MPack.xcodeproj}
cp projects/vs/mpack.{sln,vcxproj,vcxproj.filters} build/amalgamation/projects/vs
cp projects/xcode/MPack.xcodeproj/project.pbxproj build/amalgamation/projects/xcode/MPack.xcodeproj
cp src/mpack-config.h.sample build/amalgamation/src
mkdir -p build/amalgamation/tools
cp tools/gcov.sh build/amalgamation/tools
cp tools/valgrind-suppressions build/amalgamation/tools

# create package
NAME=mpack-amalgamation-$VERSION
tar -C build/amalgamation --transform "s@^@$NAME/@" -czf build/$NAME.UNTESTED.tar.gz `ls build/amalgamation` || exit $?

# build amalgamation package
if [[ "$CI" == "" ]]; then
    cd build/amalgamation
    scons -j4 all=1 || exit $?
    cd ../..
fi

# done!
mv build/$NAME.UNTESTED.tar.gz $NAME.tar.gz
echo Created $NAME.tar.gz

