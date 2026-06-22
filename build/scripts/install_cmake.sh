#!/usr/bin/env bash
set -euxo pipefail

version="3.29.0"

case "$(uname -m)" in
    x86_64)
        arch="x86_64"
        ;;
    aarch64 | arm64)
        arch="aarch64"
        ;;
    *)
        echo "unsupported CMake binary architecture: $(uname -m)"
        exit 1
        ;;
esac

archive="cmake-${version}-linux-${arch}.tar.gz"
base_url="https://cmake.org/files/v3.29"

cd /tmp
wget "${base_url}/${archive}"
wget "${base_url}/cmake-${version}-SHA-256.txt"
grep " ${archive}$" "cmake-${version}-SHA-256.txt" | sha256sum --check

install_dir="/opt/cmake-${version}"
mkdir -p "${install_dir}"
tar --extract --gzip --file="${archive}" --strip-components=1 --directory="${install_dir}"
ln --symbolic --force "${install_dir}/bin/cmake" /usr/local/bin/cmake
ln --symbolic --force "${install_dir}/bin/ctest" /usr/local/bin/ctest
ln --symbolic --force "${install_dir}/bin/cpack" /usr/local/bin/cpack

# Check installation
cmake --version
