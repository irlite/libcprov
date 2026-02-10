#!/usr/bin/env bash
set -euxo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR

mkdir -p build
pushd build

if [ -x "$(command -v module)" ]; then
    module load gcc/14.2.0
    export CC="$(which gcc)"
    export CXX="$(which g++)"
    export LDFLAGS="-ldl"
fi

cmake .. \
  -DCMAKE_CXX_STANDARD=23 \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_CXX_EXTENSIONS=ON \
  -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS:-}"

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
