#!/usr/bin/env bash
set -euxo pipefail

input_name=${1:?Need input name}
script_in="profile/out/${input_name}.perf.script"
output_svg="profile/out/${input_name}.svg"

mkdir -p profile/out
scp "dev-vm:/home/dzhiblavi/logsql/${script_in}" "${script_in}"

cat "${script_in}" | stackcollapse-perf.pl | flamegraph.pl > "${output_svg}"
