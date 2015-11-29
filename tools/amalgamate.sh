#!/bin/bash
# This script amalgamates the MPack code into a single pair of
# source files, mpack.h and mpack.c. The resulting amalgamation
# is in build/amalgamation/ (without documentation.)

. "`dirname $0`"/getversion.sh

FILES="\
    mpack-platform \
    mpack-common \
    mpack-writer \
    mpack-reader \
    mpack-expect \
    mpack-node"

TOOLS="\
    tools/clean.sh \
    tools/gcov.sh \
    tools/scan-build.sh \
    tools/valgrind-suppressions"

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
CONTENTS="test SConscript SConstruct LICENSE README.md"
cp -ar $CONTENTS build/amalgamation
mkdir -p build/amalgamation/projects/{vs,xcode/MPack.xcodeproj}
cp projects/vs/mpack.{sln,vcxproj,vcxproj.filters} build/amalgamation/projects/vs
cp projects/xcode/MPack.xcodeproj/project.pbxproj build/amalgamation/projects/xcode/MPack.xcodeproj
cp src/mpack-config.h.sample build/amalgamation/src
mkdir -p build/amalgamation/tools
cp $TOOLS build/amalgamation/tools

# done!
echo "Done. MPack amalgamation is in build/amalgamation/"

