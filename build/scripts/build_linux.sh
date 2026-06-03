#!/bin/bash
set -euxo pipefail

arch="${1:?ARCH is required}"
shift 1

if ! [[ -d "/build/lsql" ]]; then
    echo "source directory is not mounted at /build/lsql"
    exit 1
fi

cd /build/lsql

mkdir -p /output || true

for type in "${@}"; do
    echo "Building: build_type=${type}, arch=${arch}"

    make BUILD_TYPE="${type}" PROFILE="${arch}-linux.docker" build
    cp "./target/${arch}-linux.docker/${type}/src/cli/lsql" "./output/lsql-${type}-linux-${arch}"
    cp "./target/${arch}-linux.docker/${type}/src/cli/lpipe" "./output/lpipe-${type}-linux-${arch}"
done
