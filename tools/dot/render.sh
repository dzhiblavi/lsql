#!/usr/bin/env bash
set -euxo pipefail

dot_path="${1:?Need path to .dot file}"
dot_name="$(basename "${dot_path}")"
svg_path="${dot_name}.svg"

dot -Tsvg "${dot_path}" > "${svg_path}"
