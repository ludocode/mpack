#!/bin/bash
# This script amalgamates the MPack code into a single pair of
# source files, mpack.h and mpack.c. The resulting amalgamation
# is in build/amalgamation/ (without documentation.)

. "`dirname $0`"/getversion.sh

HEADERS="\
    mpack/mpack-platform.h \
    mpack/mpack-common.h \
    mpack/mpack-writer.h \
    mpack/mpack-reader.h \
    mpack/mpack-expect.h \
    mpack/mpack-node.h \
    "

SOURCES="\
    mpack/mpack-platform.c \
    mpack/mpack-common.c \
    mpack/mpack-writer.c \
    mpack/mpack-reader.c \
    mpack/mpack-expect.c \
    mpack/mpack-node.c \
    "

TOOLS="\
    tools/afl.sh \
    tools/clean.sh \
    tools/gcov.sh \
    tools/scan-build.sh \
    tools/valgrind-suppressions \
    "

FILES="\
    test \
    LICENSE \
    AUTHORS.md \
    README.md \
    CHANGELOG.md \
    "

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
echo -e "#if defined(MPACK_HAS_CONFIG) && MPACK_HAS_CONFIG" >> $HEADER
echo -e "#include \"mpack-config.h\"" >> $HEADER
echo -e "#endif\n" >> $HEADER
for f in $HEADERS; do
    echo -e "\n/* $f.h */" >> $HEADER
    sed -e 's@^#include ".*@/* & */@' -e '0,/^ \*\/$/d' src/$f >> $HEADER
done
echo -e "#endif\n" >> $HEADER

# assemble source
echo -e "#define MPACK_INTERNAL 1" >> $SOURCE
echo -e "#define MPACK_EMIT_INLINE_DEFS 1\n" >> $SOURCE
echo -e "#include \"mpack.h\"\n" >> $SOURCE
for f in $SOURCES; do
    echo -e "\n/* $f.c */" >> $SOURCE
    sed -e 's@^#include ".*@/* & */@' -e '0,/^ \*\/$/d' src/$f >> $SOURCE
done

# assemble package contents
cp -ar $FILES build/amalgamation
mkdir -p build/amalgamation/tools
cp $TOOLS build/amalgamation/tools

# done!
echo "Done. MPack amalgamation is in build/amalgamation/"
