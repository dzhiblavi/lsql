#!/usr/bin/env bash
set -euxo pipefail

program=${1:?Need program name}
threads=${2:-1}
output_name=${3:-${program}}

mkdir -p profile/out
record_out="profile/out/${output_name}.perf.data"
script_out="profile/out/${output_name}.perf.script"

perf record -F 999 -g -o "${record_out}"     \
    ./output/lsql-RelWithDebInfo-linux-amd64 \
    -l Critical -j "${threads}"              \
    "./profile/programs/${program}.sql"

perf script -i "${record_out}" > "${script_out}"
