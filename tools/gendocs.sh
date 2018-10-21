#!/bin/bash
cd "$(dirname "$0")"/..
source tools/getversion.sh
mkdir -p build/docs

# Write temporary README.md without "Build Status" section
cat README.md | \
    sed '/^## Build Status/,/^##/{//!d}' | \
    sed '/^## Build Status/d' \
    > build/docs/README.temp.md

# Doxygen gets horribly confused by headers without include guards. We cannot
# have an include guard on mpack-defaults.h because the user is encouraged to
# copy it. This would break the default fallbacks which is the whole purpose
# of the file. We add include guards here.
echo -e "#ifndef MPACK_DEFAULTS_H\n#define MPACK_DEFAULTS_H 1\n" > build/docs/mpack-defaults-doxygen.h
cat src/mpack/mpack-defaults.h >> build/docs/mpack-defaults-doxygen.h
echo -e "\n#endif" >> build/docs/mpack-defaults-doxygen.h

# Generate docs with correct version number
(
    cat docs/doxyfile
    echo "PROJECT_NUMBER = $VERSION"
    echo
) | doxygen - || exit 1

echo
echo "Docs generated: file://$(pwd)/build/docs/html/index.html"
