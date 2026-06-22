#!/usr/bin/env bash
set -euxo pipefail

version="3.29"
build="0"

cd /tmp
wget "https://cmake.org/files/v${version}/cmake-${version}.${build}.tar.gz"
tar -xzvf "cmake-${version}.${build}.tar.gz"
cd "cmake-${version}.${build}/"

# Configure and build
./bootstrap --parallel=$(nproc)
make -j$(nproc)
make install

# Check installation
cmake --version
