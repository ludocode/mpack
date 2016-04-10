#!/bin/bash
. "`dirname $0`"/getversion.sh
mkdir -p build/docs

# Write temporary README.md without "Build Status" section
cat README.md | \
    sed '/^## Build Status/,/^##/{//!d}' | \
    sed '/^## Build Status/d' \
    > build/docs/README.temp.md

# Copy mpack config to source folder so doxygen can find it
cp src/mpack-config.h.sample build/docs/mpack-config.h

# Generate docs with correct version number
(
    cat docs/doxyfile
    echo "PROJECT_NUMBER = $VERSION"
    echo
) | doxygen -
