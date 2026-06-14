#!/usr/bin/env bash
set -euo

repeat="${1}"
shift

for commit in "$@"; do
    git checkout "${commit}"
    python3 bench/run.py --build-type Release --repeat "${repeat}" --warmup 5
done
