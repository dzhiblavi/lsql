# Development

`lsql` is a query engine for text logs. It reads log files as streams of
records, extracts key/value fields, and runs query plans over them. The
command-line tool runs SQL-style queries through this engine.

The integration tests are the best executable specification. Each semantic test
contains `query.sql`, which is compared against the expected output.

## Project Layout

```text
src/front/sql      SQL style lexer, parser, AST, binder, lowering
src/front/common   shared frontend expressions and source-span helpers
src/ir             shared intermediate representation
src/optimize       IR optimization passes
src/back           planning, storage, log parsing, execution operators
src/output         output formatters
src/cli            CLI and SQL entrypoint
tests/integration  SQL behavior tests over log fixtures
tests/unit         execution, optimizer, and output unit tests
```

## Test Layout

Integration tests live under `tests/integration/semantics/`.

Each test directory normally contains:

```text
query.sql
input.*.txt
output.json
```

The SQL query is expected to produce `output.json` for the fixture.

## Continuous Integration

[CI](../.github/workflows/ci.yml) runs Linux amd64 and macOS arm64 Release builds
on pull requests and pushes to `main`. It can also be started manually from
GitHub Actions. New runs cancel earlier CI runs for the same pull request or
branch.

The builds run unit tests and semantic/error integration tests. The Linux
builder also runs `make check-format`. These builds exclude the hardening suite
with `-LE hardening`; run the full suite locally using the commands in the
[Linux](build/linux.md) or [macOS](build/mac_silicon.md) build instructions.

CI stores build archives as artifacts of the workflow run. It does not create
Git tags or GitHub Releases. To publish a release, manually run the separate
[Release workflow](../.github/workflows/release.yml) and provide the release tag
in its `tag` input, for example `v0.0.5`.
