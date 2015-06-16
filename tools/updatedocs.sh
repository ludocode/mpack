#!/bin/bash
# updates documentation in gh-pages

"`dirname $0`"/clean.sh
"`dirname $0`"/gendocs.sh || exit $?

VERSION=`grep PROJECT_NUMBER Doxyfile|sed 's@.*= *\(.*\) *@\1@'`

git checkout gh-pages || exit $?
git pull

cp -r docs/* .
rm -r docs

git add *.{html,png,js,css} search
git commit -am "Updated documentation to version $VERSION"
git show --stat
git status
