#!/usr/bin/env bash
set -euo

for commit in "$@"; do
    git checkout "${commit}"
    python3 bench/run.py --build-type Release --repeat 50 --warmup 5
done
