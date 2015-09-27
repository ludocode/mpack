#!/bin/bash
# script to merge develop to master for release. edits
# the readme to update badge branches.

[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }

tools/clean.sh
git checkout master
git merge --no-ff --no-edit develop
sed 's/branch=develop/branch=master/g' -i README.md
git add README.md
git commit --amend --no-edit
