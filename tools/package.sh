#!/bin/bash
# packages MPack up for amalgamation release

"`dirname $0`"/clean.sh
"`dirname $0`"/gendocs.sh || exit $?

VERSION=`grep PROJECT_NUMBER Doxyfile|sed 's@.*= *\(.*\) *@\1@'`

FILES="\
    mpack-platform \
    mpack-internal \
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
echo -e "#include \"mpack-config.h\"\n\n" >> $HEADER
for f in $FILES; do
    echo -e "/* $f.h */\n" >> $HEADER
    sed 's@^#include ".*@/* & */@' src/mpack/$f.h >> $HEADER
    echo -e "\n" >> $HEADER
done
echo -e "#endif\n" >> $HEADER

# assemble source
echo -e "#define MPACK_INTERNAL 1\n" >> $SOURCE
echo -e "#include \"mpack.h\"\n\n" >> $SOURCE
for f in $FILES; do
    echo -e "/* $f.c */\n" >> $SOURCE
    sed 's@^#include ".*@/* & */@' src/mpack/$f.c >> $SOURCE
    echo -e "\n" >> $SOURCE
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

# create package
NAME=mpack-amalgamation-$VERSION
tar -C build/amalgamation --transform "s@^@$NAME/@" -czf $NAME.tar.gz `ls build/amalgamation` || exit $?
echo Created $NAME.tar.gz

