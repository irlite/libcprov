#!/usr/bin/env bash
set -euxo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR

for arg in "$@"; do
    case "$arg" in
        -c|--clean)
            rm -rf -- build
            ;;
    esac
done

BUILD_MODE="normal"

for arg in "$@"; do
    case "$arg" in
        -c|--clean)
            rm -rf -- build
            ;;
        -p|--perf)
            BUILD_MODE="perf"
            ;;
    esac
done

mkdir -p build
pushd build

if type module &>/dev/null; then
    module load gcc/14.2.0
    export CC="$(which gcc)"
    export CXX="$(which g++)"
    export LDFLAGS="-ldl"
fi

if [[ "$BUILD_MODE" == "perf" ]]; then
    EXTRA_FLAGS="-fno-omit-frame-pointer -g3 -O2"
else
    EXTRA_FLAGS="-g -O0"
fi

cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_STANDARD=23 \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_CXX_EXTENSIONS=ON \
  -DCMAKE_C_FLAGS="${EXTRA_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${EXTRA_FLAGS}"

cmake --build . -- -j"$(nproc)"
popd

# Also build examples
echo $(pwd)
pushd "./test_scripts"
make
popd

# Also fix config
sed -i "s|REPLACEME|$(readlink -f ./build/injector/libinjector.so)|" ./config/config.json

popd
