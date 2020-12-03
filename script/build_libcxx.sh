#!/usr/bin/env bash

set -e

echo "building libc++/libc++abi 11 from source"
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/llvm-11.0.0.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/libcxx-11.0.0.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/libcxxabi-11.0.0.src.tar.xz

mkdir llvm-source
mkdir llvm-source/projects
mkdir llvm-source/projects/libcxx
mkdir llvm-source/projects/libcxxabi

tar -xf llvm-11.0.0.src.tar.xz -C llvm-source --strip-components=1
tar -xf libcxx-11.0.0.src.tar.xz -C llvm-source/projects/libcxx --strip-components=1
tar -xf libcxxabi-11.0.0.src.tar.xz -C llvm-source/projects/libcxxabi --strip-components=1

TARGET=`pwd`/llvm
mkdir "${TARGET}"
mkdir llvm-build
cd llvm-build

cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="${TARGET}" \
        -DLIBCXX_ABI_UNSTABLE=ON \
        ../llvm-source
make cxx -j4 VERBOSE=1
# we install into a local dir, no sudo needed
make install-cxxabi install-cxx


exit 0