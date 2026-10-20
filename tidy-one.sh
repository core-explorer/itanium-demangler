#!/bin/bash
set -e
set -u

BUILD_DIR=$1
DIR=$(dirname "$2")

cd $DIR
exec clang-tidy --quiet -p $BUILD_DIR "$2"
