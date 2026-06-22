#!/bin/bash
set -euxo pipefail

profile="${1:?PROFILE is required}"
shift 1

arch="${profile%%-*}"

if ! [[ -d "/build/lsql" ]]; then
    echo "source directory is not mounted at /build/lsql"
    exit 1
fi

cd /build/lsql

mkdir -p /output || true

for type in "${@}"; do
    echo "Building: build_type=${type}, profile=${profile}"

    make BUILD_TYPE="${type}" PROFILE="${profile}" test
    cp "./target/${profile}/${type}/src/cli/lsql" "./output/lsql-${type}-linux-${arch}"
    cp "./target/${profile}/${type}/src/cli/lpipe" "./output/lpipe-${type}-linux-${arch}"
done
