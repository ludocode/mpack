#!/bin/bash
# packages MPack up for amalgamation release

HEADERS="\
    mpack-platform.h \
    mpack-internal.h \
    mpack-common.h \
    mpack-reader.h \
    mpack-writer.h \
    mpack-expect.h \
    mpack-node.h"
SOURCES="\
    mpack-platform.c \
    mpack-internal.c \
    mpack-common.c \
    mpack-reader.c \
    mpack-writer.c \
    mpack-expect.c \
    mpack-node.c"

version=`grep PROJECT_NUMBER Doxyfile|sed 's@.*= *\(.*\) *@\1@'`

rm -rf docs
doxygen Doxyfile || exit $?

# assemble release package
#name=mpack-$version
#git archive master --prefix "$name/" > $name.tar || exit $?
#tar --transform "s@^@$name/@" -rf $name.tar docs || exit $?
#gzip -f $name.tar || exit $?
#echo
#echo Created $name.tar.gz

# build amalgamation

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
 * This is the MPack $version amalgamation package.
 *
 * http://github.com/ludocode/mpack
 */

EOF
cp $HEADER $SOURCE

# assemble header
echo -e "#ifndef MPACK_H\n#define MPACK_H 1\n" >> $HEADER
echo -e "#include \"mpack-config.h\"\n\n" >> $HEADER
for f in $HEADERS; do
    echo -e "/* $f */\n" >> $HEADER
    sed 's@^#include ".*@/* & */@' src/mpack/$f >> $HEADER
    echo -e "\n" >> $HEADER
done
echo -e "#endif\n" >> $HEADER

# assemble source
echo -e "#define MPACK_INTERNAL 1\n" >> $SOURCE
echo -e "#include \"mpack.h\"\n\n" >> $SOURCE
for f in $SOURCES; do
    echo -e "/* $f */\n" >> $SOURCE
    sed 's@^#include ".*@/* & */@' src/mpack/$f >> $SOURCE
    echo -e "\n" >> $SOURCE
done

# assemble rest of package
CONTENTS="docs test SConscript SConstruct LICENSE README.md"
cp -ar $CONTENTS build/amalgamation
cp src/mpack-config.h.sample build/amalgamation/src
mkdir -p build/amalgamation/tools
cp tools/gcov.sh build/amalgamation/tools
name=mpack-amalgamation-$version
tar -C build/amalgamation --transform "s@^@$name/@" -czf $name.tar.gz `ls build/amalgamation` || exit $?
echo Created $name.tar.gz

