#!/bin/sh

set -e
set -u
# set -x

cd "${0%/*}"

DEFAULT_RUN_TARGET="cdx-server"

. ./bld/compile_options.sh

EXTRA_CMAKE_FLAGS=""
while [ $# -gt 0 ]; do
  key="$1"
  case $key in
  *)
    CDX_parse_default_parameter "$@"
    ;;
  esac
  shift # past argument or value
done

EXTRA_CMAKE_FLAGS="${EXTRA_CMAKE_FLAGS}"

CDX_prepare_and_build


if [ "$EXEC" = "1" ]; then
  if [ "$TARGET" = "web" ]; then
    pwd
    cd ./target/latest/
    exec emrun --browser firefox ./${RUN_TARGET}.html --port $PORT
  else
    if [ -e ./$RUN_TARGET ]; then
      exec $LAUNCHER ./$RUN_TARGET $RUN_ARGS
    else
      exec $LAUNCHER ./target/latest/$RUN_TARGET $RUN_ARGS
    fi
  fi
fi
