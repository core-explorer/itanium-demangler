#!/bin/bash
#set -e
set -u
set -x

cd "$(dirname "$0")"

for s in $(pwd)/{src,test,explo,c++-examples} ; do 
	find $s \( -name "*.hxx" -o -name "*.hpp" \) -exec clang-format -i -style=file {} \;
	find $s \( -name "*.cxx*" -o -name "*.cpp" -o -name "*.c" \) -exec clang-format -i -style=file {} \;
	find $s -name "*.js" -exec js-beautify -a -r -d {} \;
    done
for s in src/web/*.cxx; do
  sed -i "s/= >/=>/g" $s
done
