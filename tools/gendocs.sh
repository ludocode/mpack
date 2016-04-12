#!/bin/bash
. "`dirname $0`"/getversion.sh
mkdir -p build/docs

# Write temporary README.md without "Build Status" section
cat README.md | \
    sed '/^## Build Status/,/^##/{//!d}' | \
    sed '/^## Build Status/d' \
    > build/docs/README.temp.md

# Generate docs with correct version number
(
    cat docs/doxyfile
    echo "PROJECT_NUMBER = $VERSION"
    echo
) | doxygen -
