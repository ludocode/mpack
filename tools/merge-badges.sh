#!/bin/bash
# A wrapper around git merge that updates the
# branch for README.md badges
[[ -z $(git status --porcelain) ]] || { git status --porcelain; echo "Tree is not clean!" ; exit 1; }
git merge --no-ff --no-edit $1
branch=`git rev-parse --abbrev-ref HEAD`
sed "s@branch\([=/]\)$1@branch\1$branch@g" -i README.md
git add README.md
git commit --amend --no-edit
