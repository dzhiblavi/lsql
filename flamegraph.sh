#!/usr/bin/env bash
set -euxo pipefail

cat out.perf | stackcollapse-perf.pl | flamegraph.pl
