#!/usr/bin/env bash

set -e

echo "building the latest libcxx from source..."
git clone https://github.com/llvm/llvm-project

INSTALL_TARGET=`pwd`/local
mkdir "${INSTALL_TARGET}"
cd llvm-project

cmake \
  -S libcxx \
  -B buildcxx \
  -D CMAKE_C_COMPILER=${CC} \
  -D CMAKE_CXX_COMPILER=${CXX} \
  -D "CMAKE_INSTALL_PREFIX=${INSTALL_TARGET}" \
  -D CMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build buildcxx -j2 -v
cmake --build buildcxx --target install

cmake \
  -S libcxxabi \
  -B buildcxxabi \
  -D CMAKE_C_COMPILER=${CC} \
  -D CMAKE_CXX_COMPILER=${CXX} \
  -D "CMAKE_INSTALL_PREFIX=${INSTALL_TARGET}" \
  -D CMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build buildcxxabi -j2 -v
cmake --build buildcxxabi --target install

exit 0