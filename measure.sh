#!/usr/bin/env bash
set -euxo pipefail

threads=${1:-1}

perf record -F 999 -g ./output/lsql-RelWithDebInfo-linux-amd64 prog.sql -l Critical -j "${threads}"
perf script > out.perf
