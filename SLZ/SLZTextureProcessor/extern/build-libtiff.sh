#!/bin/bash

mode=$1
if [ -z $1 ]; then
	mode=Release
fi

cd ./libtiff
cmake \
    -B build-linux \
    -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="-fPIC" \
    -DCMAKE_CXX_FLAGS="-fPIC" \
    -Dtiff-static=ON \
    -Dtiff-tests=OFF \
    -Dtiff-contrib=OFF \
    -Dtiff-docs=OFF \
    -Dtiff-install=OFF \
    -DCMAKE_BUILD_TYPE=$mode

cd ./build-linux

cp ./libtiff/libtiff.a ../../../lib/libtiff/Linux-x86_64/libtiff.a


ninja