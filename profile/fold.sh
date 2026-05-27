#!/usr/bin/env bash
set -euxo pipefail

for file in $(ls *.folded); do
    flamegraph.pl "${file}" > "${file}.svg"
done
