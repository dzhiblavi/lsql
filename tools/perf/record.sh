#!/usr/bin/env bash
set -euxo pipefail

# record.sh <name> <program args...>

name=${1:?Need record name}
shift 1

record_out="${name}.perf.data"
script_out="${name}.perf.script"

perf record -F 999 -g -o "${record_out}" "$@"
perf script -i "${record_out}" >"${script_out}"
