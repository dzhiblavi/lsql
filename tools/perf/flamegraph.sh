#!/usr/bin/env bash
set -euxo pipefail

# flamegraph.sh <name.perf>

input_name=${1:?Need input name (e.g. program.perf)}
script_in="${input_name}.script"
output_svg="${input_name}.svg"

cat "${script_in}" | stackcollapse-perf.pl | flamegraph.pl > "${output_svg}"
