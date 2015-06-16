#!/bin/bash
# converts the unsupported syntax highlighting lines in the README.md from
# github-flavored markdown style to standard markdown before generating documentation

[[ -z $(git status -uno --porcelain) ]] || { git status -uno --porcelain; echo "Cannot generate docs, tree is not clean!" ; exit 1; }
sed '/^```C/,/^```$/ s/^[^`]/    &/' -i README.md
sed '/^```/d' -i README.md
doxygen
RET=$?
git checkout README.md
exit $RET
