#!/bin/bash
# updates documentation in gh-pages

"`dirname $0`"/clean.sh
. "`dirname $0`"/gendocs.sh
cp -ar docs/html docs-html
"`dirname $0`"/clean.sh

git checkout gh-pages || exit $?
git pull

rm -r *.{html,png,js,css} search
cp -r docs-html/* .
rm -r docs-html

git add *.{html,png,js,css} search
git commit -am "Updated documentation to version $VERSION"
git show --stat
git status
