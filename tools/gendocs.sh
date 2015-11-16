#!/bin/bash

. "`dirname $0`"/getversion.sh

[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }

# Insert version number into Doxyfile
sed "s/^\(PROJECT_NUMBER =\) develop/\1 $VERSION/" -i Doxyfile

# Convert the unsupported syntax highlighting lines in the README.md from
# github-flavored markdown style to standard markdown
sed '/^```C/,/^```$/ s/^[^`]/    &/' -i README.md
sed '/^```/d' -i README.md
sed '/travis-ci\.org\/ludocode\/mpack\.svg/d' -i README.md

# Generate docs
doxygen
DOXYGEN_RET=$?
git checkout README.md Doxyfile || exit $?
if [ "$DOXYGEN_RET" -ne 0 ]; then
    exit $DOXYGEN_RET
fi
