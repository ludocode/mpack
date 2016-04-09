#!/bin/bash
. "`dirname $0`"/getversion.sh

# Write temporary README.md without "Build Status" section
cat README.md | \
    sed '/^## Build Status/,/^##/{//!d}' | \
    sed '/^## Build Status/d' \
    > README.temp.md

# Generate docs with edited README.md and correct version number
mkdir -p build
(
    cat docs/doxyfile | sed -e "s/README\.md/README.temp.md/"
    echo "PROJECT_NUMBER = $VERSION"
    echo "USE_MDFILE_AS_MAINPAGE = README.temp.md"
    echo
) | doxygen -

RET=$?
rm README.temp.md
(exit $RET)
