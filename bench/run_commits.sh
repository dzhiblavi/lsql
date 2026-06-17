#!/usr/bin/env bash
set -euo

for commit in "$@"; do
    git checkout "${commit}"
    python3 bench/run.py --build-type Release --warmup 2 --time-limit 10
done
