#!/usr/bin/env bash
set -euxo pipefail

llvm_version=${1:?LLVM version required}

# Download llvm install script
wget -O - https://apt.llvm.org/llvm.sh > ./llvm.sh

# Install the requested version of LLVM toolchain (libc++ included)
chmod +x ./llvm.sh
./llvm.sh "${llvm_version}" all
