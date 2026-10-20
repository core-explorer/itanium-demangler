#!/bin/bash
set -e
set -u
#set -x

cd "$(dirname "$0")"
BUILD_DIR=$(realpath $1)
for s in $(pwd)/src/ ; do 
  nice -n 5 find $s \( -name "*.cxx"  -a -not -name "*-shell.cxx" \) -exec ./tidy-one.sh $BUILD_DIR {} \; 
done

