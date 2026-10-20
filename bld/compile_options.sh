#!/bin/sh

# BASH SAFEGUARDS
set -e # Exit immediately if a pipeline/cmd returns non-zero
set -u # Treat unset variables and parameters
#set -v # Print shell input lines as they are read
#set -x # print commands as they are executed

BUILDDIR_ORIG=$(pwd)
BUILDDIR_BASE="."

TOOLCHAIN_DIR=.
NATIVE_TARGET=$(uname -m)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)/bld
TOOLCHAIN_GCC="Toolchain-gcc.cmake"
TOOLCHAIN_CLANG="Toolchain-clang.cmake"
TOOLCHAIN_AFL="Toolchain-afl.cmake"
TOOLCHAIN_WASM32="Toolchain-wasm32.cmake"
TOOLCHAIN_WASM64="Toolchain-wasm64.cmake"

TARGET_PROCESSOR=$NATIVE_TARGET

# DEFAULT PARAMETERS
if [ -z ${CDX_BUILD_THREADS+x} ]; then
  CDX_BUILD_THREADS=$(($(nproc) * 3 / 4))
fi
THREADS=$CDX_BUILD_THREADS
BUILD_TYPE=RelWithDebInfo
TOOLCHAIN_FILE=$TOOLCHAIN_GCC
TARGET=$NATIVE_TARGET
LTO_FLAGS=""
VERBOSE=
BUILDSUFFIX=""
EXTRA_CMAKE_FLAGS=""
NO_PACKAGE="FALSE"
TOOLCHAIN_ROOT=${SCRIPT_DIR}
CMAKE_WARNING_FLAGS=""
_CDX_skip_next="FALSE"
COVERAGE_FLAGS=""
SINGLE_TARGET=""
OPTIONAL_BUILD_FLAGS=""
SYMLINK_BUILDDIR="FALSE"
SCP_TARGET=""
PROJECT_OPTIONS=""
CMAKE_FLAGS_INTERNAL=""
CD_INTO_BUILDDIR="0"
EXEC="0"
COMPILE_FLAGS=""
LINK_FLAGS=""
if [ -z ${CDX_EXTRA_TAG+x} ]; then
  BUILDSUFFIX_PARTS=""
else
  BUILDSUFFIX_PARTS="${CDX_EXTRA_TAG}"
fi
PORT=8000
LAUNCHER=""
RUN_ARGS=""
DBG_LAUNCHER_FAIL="gdb -ex=run -ex=bt --args"
DBG_LAUNCHER="gdb -ex=run -ex=quit --args"
BUILD_TOOL_OPTIONS=""
RUN_TARGET=""
EMSCRIPTEN_ENV=""
HARDENING="none"
STATIC_ANALYSIS="0"

UNITY_BUILD="FALSE"
if [ -z ${APPLICATION+x} ]; then
  APPLICATION="project"
  PROJECT_LINE=$(grep -i project CMakeLists.txt | head -n1 | sed "s/[Pp][Rr][Oo][Jj][Ee][Cc][Tt](\(.*\))/\\1/" | cut -d " " -f1)
  if [ ${PROJECT_LINE} != "" ]; then
    APPLICATION=$PROJECT_LINE
  fi
fi

CDX_fix_path() {
  true
}

CDX_generate_cmake_flags() {
  TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT/$TOOLCHAIN_FILE
  CMAKE_FLAGS="${CMAKE_WARNING_FLAGS} -DCMAKE_BUILD_TYPE=$BUILD_TYPE $TOOLCHAIN ${CMAKE_FLAGS_INTERNAL} ${EXTRA_CMAKE_FLAGS} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
}

CDX_parse_default_parameter() {
  key="$1"
  if [ $_CDX_skip_next != "TRUE" ]; then
    # READ COMMAND LINE PARAMS
    case $key in
    --asan)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_ASAN=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} asan"
      export ASAN_OPTIONS="abort_on_error=1"
      ;;
      --analyzer)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_ANALYZER=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} analyzer"
      ;;
    -p | --port)
      PORT=$2
      _CDX_skip_next="TRUE"
      ;;
    -pthread)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_FORCE_PTHREAD=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} pthread"
      ;;
    -b | --just-build)
      SINGLE_TARGET="$SINGLE_TARGET $2"
      RUN_TARGET="$2"
      NO_PACKAGE="TRUE"
      _CDX_skip_next="TRUE"
      ;;
      --hardening)
      HARDENING="$2"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_HARDENING=$HARDENING"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} hardening-$HARDENING"
      _CDX_skip_next="TRUE"
      ;;
    --build-dir)
      BUILDDIR_BASE="$2"
      mkdir -p "$BUILDDIR_BASE"
      CMAKE_ROOT=${BUILDDIR_ORIG}
      TOOLCHAIN_ROOT=${BUILDDIR_ORIG}
      _CDX_skip_next="TRUE"
      ;;
    -c | --clang)
      TOOLCHAIN_FILE=$TOOLCHAIN_CLANG
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} clang"
      ;;
    --afl)
      TOOLCHAIN_FILE=$TOOLCHAIN_AFL
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} afl"
      ;;
    -e | --wasm32)
      TOOLCHAIN_FILE=$TOOLCHAIN_WASM32
      TARGET="wasm32"
      TARGET_PROCESSOR="$TARGET"
      ;;
    --web)
      TOOLCHAIN_FILE=$TOOLCHAIN_WASM32
      TARGET="wasm32"
      TARGET_PROCESSOR="$TARGET"
      EMSCRIPTEN_ENV="web"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} web"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DEMSCRIPTEN_ENV=web"
      ;;
    --node)
      TOOLCHAIN_FILE=$TOOLCHAIN_WASM32
      TARGET="wasm32"
      TARGET_PROCESSOR="$TARGET"
      EMSCRIPTEN_ENV="node"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} node"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DEMSCRIPTEN_ENV=node"
      ;;
    --assert)
    BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} assert"
    CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_ASSERT=ON"
    ;;
    -E | --wasm64)
      TOOLCHAIN_FILE=$TOOLCHAIN_WASM64
      TARGET="wasm64"
      TARGET_PROCESSOR="$TARGET"

      ;;
    -d | --debug)
      BUILD_TYPE=Debug
      ;;

    -j)
      THREADS="$2"
      _CDX_skip_next="TRUE"
      ;;
    -j1)
      THREADS="1"
      ;;
    -L | --lto)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_LTO=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} lto"
      ;;
    --msr)
      BUILD_TYPE=MinSizeRel
      ;;
    --unity | -u)
      UNITY_BUILD="TRUE"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} unity"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_UNITY=ON"
      ;;
    --gprof)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_PROFILE=MEASURE"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} gprof"
      ;;
    --pgo-collect)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_PROFILE=COLLECT"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} pgo"
      ;;
    --pgo-use)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_PROFILE=USE"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} pgo"
      ;;
    --fatal)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_FATAL_ERRORS=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} fatal"
      ;;
    --no-error)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_WNO_ERROR=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} noerror"
      ;;
    --rwd)
      BUILD_TYPE=RelWithDebInfo
      ;;
    -r | --release)
      BUILD_TYPE=Release
      ;;
    -z | --ram)
      BUILDDIR_BASE=/dev/shm/$APPLICATION
      mkdir -p "$BUILDDIR_BASE"
      CMAKE_ROOT=${BUILDDIR_ORIG}
      TOOLCHAIN_ROOT=${BUILDDIR_ORIG}
      SYMLINK_BUILDDIR="TRUE"
      # BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} ram"
      ;;
    -t | --tag)
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} $2"
      _CDX_skip_next="TRUE"
      ;;
    -u | --ubsan)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_UBSAN=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} ubsan"
      ;;
    --msan)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_MSAN=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} msan"
      ;;
    -p |--pthread)
          CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_EMSCRIPTEN_PTHREAD=ON"
          BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} pthread"
          ;;
    -v | --verbose)
      VERBOSE="VERBOSE=1"
      ;;
    -w | --work-dir)
      #CD_INTO_BUILDDIR="1"
      EXEC="1"
      ;;
    -wd | -dw)
      EXEC="1"
      BUILD_TYPE=Debug
      ;;
    -x)
      LAUNCHER="$LAUNCHER $2"
      _CDX_skip_next="TRUE"
      #CD_INTO_BUILD_DIR="1"
      EXEC="1"
      ;;
    -dx | -xd | -XD)
      LAUNCHER="${DBG_LAUNCHER}"
      BUILD_TYPE=Debug
      EXEC="1"
      ;;
    -dxf | -xdf)
      LAUNCHER="${DBG_LAUNCHER_FAIL}"
      BUILD_TYPE=Debug
      EXEC="1"
      ;;
    -rdx | --rwdx | -xrd)
      LAUNCHER="${DBG_LAUNCHER}"
      BUILD_TYPE=RelWithDebInfo
      EXEC="1"
      ;;
    -rdxf | --rwdxf | -xrdf)
      LAUNCHER="${DBG_LAUNCHER_FAIL}"
      BUILD_TYPE=RelWithDebInfo
      EXEC="1"
      ;;
    --arg)
      RUN_ARGS="$RUN_ARGS $2"
      _CDX_skip_next="TRUE"
      #CD_INTO_BUILD_DIR="1"
      EXEC="1"
      ;;
    -adx | -axd | -xda | dxa)
      LAUNCHER="${DBG_LAUNCHER}"
      EXEC="1"
      BUILD_TYPE="Debug"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_ASAN=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} asan"
      export ASAN_OPTIONS="abort_on_error=1:detect_leaks=0"
      ;;
    --warn-no-dev)
      CMAKE_WARNING_FLAGS="-Wno-dev"
      ;;
    -g | --gen-dbg)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DCDX_ENABLE_DBG=ON"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} dbg"
      ;;
    --scp | -s)
      SCP_TARGET="$2"
      _CDX_skip_next="TRUE"
      ;;
    --aarch64)
      TARGET="aarch64"
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DTARGET_PROCESSOR=aarch64"
      ;;
    --gcc)
      CMAKE_FLAGS_INTERNAL="${CMAKE_FLAGS_INTERNAL} -DGCCVER=$2"
      BUILDSUFFIX_PARTS="${BUILDSUFFIX_PARTS} gcc$2"
      _CDX_skip_next="TRUE"
      ;;
    -D)
      EXTRA_CMAKE_FLAGS="${EXTRA_CMAKE_FLAGS} -D$2"
      _CDX_skip_next="TRUE"
      ;;
    -k | --keep-going)
      BUILD_TOOL_OPTIONS="${BUILD_TOOL_OPTIONS} -k"
      ;;
    --trace)
      set -x
      ;;
    --help)
      echo known common options:
      echo --asan --ubsan -j "<NUM_THREADS>" --debug --rwd --release --msr --ram --tag "<TAG>" --no-dbg --verbose --warn-no-dev --no-package --lto --clang --no-strip -D
      echo ${OPTIONAL_BUILD_FLAGS}
      exit 0
      ;;
    *)
      # unknown option
      echo unknown option \"$key\"
      exit 1
      ;;
    esac
  else
    _CDX_skip_next="FALSE"
  fi
}

CDX_generate_build_dir() {
  # BUILDDIR NAME
  BUILDSUFFIX=$(echo $BUILDSUFFIX_PARTS | xargs -n1 | sort | xargs | sed "s/ /-/g")
  if [ "$BUILDSUFFIX" != "" ]; then
    BUILDSUFFIX="-${BUILDSUFFIX}"
  fi
  #if [ ! -e $HOME/local_builds ] ; then mkdir $HOME/local_builds ; fi
  if [ ! -e $BUILDDIR_BASE/target ]; then
    #mkdir -p $HOME/local_builds/$APPLICATION
    mkdir -p $BUILDDIR_BASE/target
    #ln -s $HOME/local_builds/$APPLICATION $BUILDDIR_BASE/target
  fi
  BUILD_DIR=$BUILDDIR_BASE/target/"$TARGET"-"$BUILD_TYPE""${BUILDSUFFIX}"
  if [ "$BUILD_TYPE" != "web" ]; then
    rm -f "${BUILDDIR_BASE}/target/latest"
    ln -s ./"$TARGET"-"$BUILD_TYPE""${BUILDSUFFIX}" "${BUILDDIR_BASE}/target/latest"
  fi
}

CDX_apply_docker_settings() {
  true
}

CDX_create_directories() {
  # CREATE DIRECTORIES
  if [ ! -d $BUILD_DIR ]; then
    mkdir -p $BUILD_DIR
  fi
}

CDX_prepare_build_dir() {

  # PREPARE BUILD DIRECTORY
  if [ -z ${CMAKE_FLAGS+x} ]; then
    CDX_generate_cmake_flags
  fi

  if [ ${SYMLINK_BUILDDIR} = "TRUE" ]; then
    if [ ! -e $BUILDDIR_ORIG/target/"$TARGET"-"$BUILD_TYPE""${BUILDSUFFIX}"-ram ]; then
      ln -sfv $BUILDDIR_BASE/target/"$TARGET"-"$BUILD_TYPE""${BUILDSUFFIX}" $BUILDDIR_ORIG/target/"$TARGET"-"$BUILD_TYPE""${BUILDSUFFIX}"-ram
    fi
  fi

}

CDX_run_build() {

  SAVED_DIR=$(pwd -P)
  if [ ! -e Makefile ]; then
    nice -n 10 cmake ${CMAKE_FLAGS} $BUILDDIR_ORIG ${PROJECT_OPTIONS}
  fi
  CT=""
  for b in $SINGLE_TARGET; do
    CT="$CT -t $b"
  done
  nice -n 10 cmake --build . $CT -- $VERBOSE -j $THREADS ${BUILD_TOOL_OPTIONS}
  cd "$SAVED_DIR"

  if [ ! -z "$SCP_TARGET" ]; then
    for target in $SINGLE_TARGET; do
      CDX_fix_path target
      scp $target $SCP_TARGET
    done
  fi
}

CDX_fix_perms() {
  true
}

CDX_prepare_and_build() {
  CDX_generate_build_dir
  CDX_apply_docker_settings
  CDX_create_directories

  if [ "$RUN_TARGET" = "" ]; then
    if [ "$TARGET" = "web" ]; then
      RUN_TARGET=index
    else
      RUN_TARGET=$DEFAULT_RUN_TARGET
    fi
  fi
  SAVED_DIR=$(pwd -P)
  cd $BUILD_DIR/


  CDX_prepare_build_dir
  CDX_run_build
  CDX_fix_perms
  if [ $CD_INTO_BUILDDIR = "0" ]; then
    cd "$SAVED_DIR"
  fi
}
