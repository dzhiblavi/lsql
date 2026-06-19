#!/usr/bin/env bash
set -euo

for commit in "$@"; do
    git checkout "${commit}"
    make build BUILD_TYPE=Release TESTS=OFF
    tag="$(git rev-parse --short=12 HEAD)"
    if ! git diff --quiet; then
        tag="${tag}-dirty"
    fi
    python3 bench/run.py --binary ./output/lsql-Release --tag "${tag}" --warmup 2 --time-limit 10
done
