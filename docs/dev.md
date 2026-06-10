# Development

The integration tests are the best executable specification. Each semantic test
contains both `query.sql` and `query.pipe`, and both are compared against the
same expected output.

## Project Layout

```text
src/front/sql      SQL style lexer, parser, AST, binder, lowering
src/front/pipe     unix pipe style lexer, parser, AST, binder, lowering
src/front/common   shared frontend expressions and source-span helpers
src/ir             shared intermediate representation
src/optimize       IR optimization passes
src/back           planning, storage, log parsing, execution operators
src/output         output formatters
src/cli            shared CLI and frontend-specific entrypoints
tests/integration  SQL and pipe behavior tests over log fixtures
tests/exec         execution-operator unit tests
```

## Test Layout

Integration tests live under `tests/integration/semantics/`.

Each test directory normally contains:

```text
query.sql
query.pipe
input.*.txt
output.json
```

The SQL and pipe queries are expected to produce equivalent output for the same
fixture.
