#!/usr/bin/env bash
set -euo

for commit in "$@"; do
    {
        git checkout "${commit}"
        make BUILD_TYPE=Release build
        git checkout -
    }
    python3 bench/run.py --build-type Release --skip-build --warmup 2
done
