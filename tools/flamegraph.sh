#!/usr/bin/env bash
set -euxo pipefail

# flamegraph.sh <name.folded>

folded_path="${1:?Need path to fold file}"
folded_name=$(basename "${folded_path}")

flamegraph.pl "${folded_path}" > "${folded_name}.svg"
