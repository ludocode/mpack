#!/bin/bash

[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }

# Insert version number into Doxyfile
MAJOR=`grep '^#define MPACK_VERSION_MAJOR' src/mpack/mpack-common.h|sed 's/.* \([0-9][0-9]*\) .*/\1/'`
MINOR=`grep '^#define MPACK_VERSION_MINOR' src/mpack/mpack-common.h|sed 's/.* \([0-9][0-9]*\) .*/\1/'`
PATCH=`grep '^#define MPACK_VERSION_PATCH' src/mpack/mpack-common.h|sed 's/.* \([0-9][0-9]*\) .*/\1/'`
VERSION="$MAJOR.$MINOR"
if [[ "$PATCH" -gt 0 ]]; then
    VERSION="$VERSION.$PATCH"
fi
sed "s/^\(PROJECT_NUMBER =\) develop/\1 $VERSION/" -i Doxyfile

# Convert the unsupported syntax highlighting lines in the README.md from
# github-flavored markdown style to standard markdown
sed '/^```C/,/^```$/ s/^[^`]/    &/' -i README.md
sed '/^```/d' -i README.md
sed '/travis-ci\.org\/ludocode\/mpack\.svg/d' -i README.md

# Generate docs
doxygen
RET=$?
git checkout README.md Doxyfile
(exit $RET)
