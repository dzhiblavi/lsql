#!/bin/bash
set -euxo pipefail

arch="${1:?ARCH is required}"
shift 1

cd /build
mkdir -p /output || true

for type in "${@}"; do
    echo "Building: build_type=${type}, arch=${arch}"

    make BUILD_TYPE="${type}" PROFILE="${arch}-linux.docker" build
    cp "/build/target/${arch}-linux.docker/${type}/src/cli/lsql" "/output/lsql-${type}-linux-${arch}"
done
