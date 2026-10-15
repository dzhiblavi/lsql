# Development

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
