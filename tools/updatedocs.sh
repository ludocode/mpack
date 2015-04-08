#!/bin/bash
# updates documentation in gh-pages
"`dirname $0`"/clean.sh
version=`grep PROJECT_NUMBER Doxyfile|sed 's@.*= *\(.*\) *@\1@'`
doxygen || exit $?
git checkout gh-pages || exit $?
git pull
cp -r docs/* .
rm -r docs
git add *.{html,png,js,css} search
git commit -am "Updated documentation to version $version"
git show --stat
git status
