# Release notes

## v0.0.6 (WIP)

- Added explicit time format overrides for file timestamp ranges.

## v0.0.5

- Made COUNT case-insensitive, consistent with other function names.
- Added macro expression support.
- Cache repeated scalar expressions within projections.
- Deduplicate aggregates in aggregate SELECT and GROUP BYs.
- Normalize COUNT of non-null constants to COUNT(*).
- Simplify redundant casts and COALESCE expressions.
- Cache repeated scalar expressions within filters.
- Cache repeated scalar arguments across aggregates.
- Hardened log format detection and added headerless CSV and TSV input.
- Added JSON Lines root fields as nullable strings.

## v0.0.4

- Filter pushdown through left join.
- Filter pushdown through projections.
- Filter pushdown through sorts.
- Filter pushdown through unions.

## v0.0.3

- Hardening tests and frontend bug fixes.
- `--no-buffer` flag support.
- Added optimization: collapsing UnionAll tree into single operation.
- Allowed to dynamically override most of the settings.

## v0.0.2

- Removed pipe frontend.
- Added simplistic JOIN support.
- Applied minor fixes related to corner cases and diagnostics.

## v0.0.1

Commit: `36e29c5a9e8fd8a0e3be066add8d7e3be34d08d3`

- Added `JSON_VALUE` support to SQL and pipe queries.
- Added direct querying of JSON Lines input.
- Improved pretty-table rendering for structured and Unicode values.
