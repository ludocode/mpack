#!/bin/bash
cd "$(dirname "$0")"/..
source tools/getversion.sh
mkdir -p .build/docs

# Write temporary README.md without "Build Status" section
cat README.md | \
    sed '/^## Build Status/,/^##/{//!d}' | \
    sed '/^## Build Status/d' \
    > .build/docs/README.temp.md

# Generate docs with correct version number
(
    cat docs/doxyfile
    echo "PROJECT_NUMBER = $VERSION"
    echo
) | doxygen - || exit 1

echo
echo "Docs generated: file://$(pwd)/.build/docs/html/index.html"
