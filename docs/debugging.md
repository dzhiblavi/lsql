# Debugging Queries

Use the diagnostic flags to follow a query from SQL syntax through binding,
optimization, and execution planning.

| Flag | Stage | What to inspect |
| --- | --- | --- |
| `--print-ast` | Abstract syntax tree (AST) | How the parser understood the query. |
| `--print-bound` | Bound AST | Resolved field names, types, and expression dependencies. |
| `--print-ir-unoptimized` | Intermediate representation (IR) | Relational operations before optimization. |
| `--print-optimize-report` | Optimizer report | Changes made by each optimization pass. |
| `--print-ir-optimized` | Optimized IR | Relational operations after optimization. |
| `--explain` | Execution plan | Execution operators, schemas, dependencies, and phases. |

These flags print to stdout. By default, they suppress query execution, but
parsing, binding, optimization, and planning still run. An error in a later
stage can therefore appear after an earlier stage's dump. Add `--run` to also
execute the query.

The examples use `./lsql`. For a local build, substitute
`./output/lsql-Debug` or `./output/lsql-Release`. See the
[CLI reference](cli.md) for all options.

## Example Files

Save these lines as `app.log`:

```text
[2026-May-06 12:00:01.123456] uid=aaa status=ok
[2026-May-06 12:00:02.654321] uid=bbb status=fail
```

Save this query as `query.sql` in the same directory:

```sql
SELECT uid, 1 + 2 AS total
FROM app.log
LIMIT 1
```

The query returns the first record's `uid` and the value of `1 + 2`. This makes
field binding, constant folding, and limit pushdown visible in the dumps below.
The examples show actual output; internal IDs, field ordering, and dump layouts
can change between versions.

## AST: Check Parsing

```sh
./lsql --print-ast query.sql
```

The AST preserves the SQL structure: a `SelectRelation`, two projectors, the
`app.log` source, and `LIMIT 1`. The addition is a `BinaryExpr` with type `Plus`
and two integer literals. The `span` values point back to the query source.
Field names have not yet been resolved to field IDs.

<details>
<summary>Example output</summary>

```text
AST dump:
Program AST
-  type: Statement
|  node:
|  |  type: QueryStatement
|  |  relation:
|  |  |  type: Relation
|  |  |  node:
|  |  |  |  type: SelectRelation
|  |  |  |  projectors:
|  |  |  |  |  -  type: Projector
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: IdentifierProjector
|  |  |  |  |  |  |  identifier: uid
|  |  |  |  |  |  span: 0:7-0:10
|  |  |  |  |  -  type: Projector
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: ExprProjector
|  |  |  |  |  |  |  alias: total
|  |  |  |  |  |  |  expr:
|  |  |  |  |  |  |  |  type: Expr
|  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  type: BinaryExpr
|  |  |  |  |  |  |  |  |  type: Plus
|  |  |  |  |  |  |  |  |  left:
|  |  |  |  |  |  |  |  |  |  type: Expr
|  |  |  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  |  |  type: LiteralExpr
|  |  |  |  |  |  |  |  |  |  |  literal:
|  |  |  |  |  |  |  |  |  |  |  |  type: Literal
|  |  |  |  |  |  |  |  |  |  |  |  type: Integer
|  |  |  |  |  |  |  |  |  |  |  |  value_str: 1
|  |  |  |  |  |  |  |  |  |  |  |  span: 0:12-0:13
|  |  |  |  |  |  |  |  |  |  span: 0:12-0:13
|  |  |  |  |  |  |  |  |  right:
|  |  |  |  |  |  |  |  |  |  type: Expr
|  |  |  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  |  |  type: LiteralExpr
|  |  |  |  |  |  |  |  |  |  |  literal:
|  |  |  |  |  |  |  |  |  |  |  |  type: Literal
|  |  |  |  |  |  |  |  |  |  |  |  type: Integer
|  |  |  |  |  |  |  |  |  |  |  |  value_str: 2
|  |  |  |  |  |  |  |  |  |  |  |  span: 0:16-0:17
|  |  |  |  |  |  |  |  |  |  span: 0:16-0:17
|  |  |  |  |  |  |  |  span: 0:12-0:17
|  |  |  |  |  |  span: 0:12-0:26
|  |  |  |  source:
|  |  |  |  |  type: Relation
|  |  |  |  |  node:
|  |  |  |  |  |  type: FileRelation
|  |  |  |  |  |  path: app.log
|  |  |  |  |  span: 1:6-1:13
|  |  |  |  limit:
|  |  |  |  |  type: Limit
|  |  |  |  |  limit: 1
|  |  |  |  |  span: 2:1-2:8
|  |  |  span: 0:0-2:8
|  span: 0:0-2:8
```

</details>

## Bound AST: Check Names and Types

```sh
./lsql --print-bound query.sql
```

Binding resolves `uid` to `uid(1,String)` and the computed output column to
`total(2,Integer)`. A field label contains its name, internal ID, and type.
The addition now has type `Add` and result type `Integer`; `level: Const` and
`required_fields: []` show that it does not depend on an input record.

Use this stage to check which fields are visible and whether expressions have
the types you expect. File fields such as `uid` are strings; see
[input field types](io.md#log-input).

<details>
<summary>Example output</summary>

```text
Bound AST dump:
Bound AST
-  type: QueryStatement
|  relation:
|  |  type: Relation
|  |  node:
|  |  |  type: SelectRelation
|  |  |  projectors:
|  |  |  |  -  type: IdentifierProjector
|  |  |  |  |  field_id: uid(1,String)
|  |  |  |  -  type: ExprProjector
|  |  |  |  |  alias_field_id: total(2,Integer)
|  |  |  |  |  expr:
|  |  |  |  |  |  type: Expr
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: BinaryExpr
|  |  |  |  |  |  |  type: Add
|  |  |  |  |  |  |  left:
|  |  |  |  |  |  |  |  type: Expr
|  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  type: ValueExpr
|  |  |  |  |  |  |  |  |  value: 1
|  |  |  |  |  |  |  |  value_type: Integer
|  |  |  |  |  |  |  |  level: Const
|  |  |  |  |  |  |  |  required_fields: []
|  |  |  |  |  |  |  |  span:
|  |  |  |  |  |  |  |  |  type: SourceSpan
|  |  |  |  |  |  |  |  |  begin:
|  |  |  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  |  |  column: 12
|  |  |  |  |  |  |  |  |  end:
|  |  |  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  |  |  column: 13
|  |  |  |  |  |  |  right:
|  |  |  |  |  |  |  |  type: Expr
|  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  type: ValueExpr
|  |  |  |  |  |  |  |  |  value: 2
|  |  |  |  |  |  |  |  value_type: Integer
|  |  |  |  |  |  |  |  level: Const
|  |  |  |  |  |  |  |  required_fields: []
|  |  |  |  |  |  |  |  span:
|  |  |  |  |  |  |  |  |  type: SourceSpan
|  |  |  |  |  |  |  |  |  begin:
|  |  |  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  |  |  column: 16
|  |  |  |  |  |  |  |  |  end:
|  |  |  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  |  |  column: 17
|  |  |  |  |  |  value_type: Integer
|  |  |  |  |  |  level: Const
|  |  |  |  |  |  required_fields: []
|  |  |  |  |  |  span:
|  |  |  |  |  |  |  type: SourceSpan
|  |  |  |  |  |  |  begin:
|  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  column: 12
|  |  |  |  |  |  |  end:
|  |  |  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  |  |  line: 0
|  |  |  |  |  |  |  |  column: 17
|  |  |  source:
|  |  |  |  type: Relation
|  |  |  |  node:
|  |  |  |  |  type: FileRelation
|  |  |  |  |  path: app.log
|  |  |  |  fields_out: fields: [uid(1,String)]
|  |  |  |  span:
|  |  |  |  |  type: SourceSpan
|  |  |  |  |  begin:
|  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  line: 1
|  |  |  |  |  |  column: 6
|  |  |  |  |  end:
|  |  |  |  |  |  type: SourcePos
|  |  |  |  |  |  line: 1
|  |  |  |  |  |  column: 13
|  |  |  limit:
|  |  |  |  type: Limit
|  |  |  |  limit: 1
|  |  |  aggregate: 0
|  |  fields_out: fields: [uid(1,String),total(2,Integer)]
|  |  span:
|  |  |  type: SourceSpan
|  |  |  begin:
|  |  |  |  type: SourcePos
|  |  |  |  line: 0
|  |  |  |  column: 0
|  |  |  end:
|  |  |  |  type: SourcePos
|  |  |  |  line: 2
|  |  |  |  column: 8
```

</details>

## Unoptimized IR: Check Lowering

```sh
./lsql --print-ir-unoptimized query.sql
```

Lowering turns the SQL-shaped AST into separate relational operations. The
outer `LimitRelation` reads from a `ProjectionRelation`, which reads `app.log`.
The `total` projector still contains an `Add` call with arguments `1` and `2`.
Compare this dump with the bound AST when investigating how SQL constructs
become relational operations.

<details>
<summary>Example output</summary>

```text
Unoptimized IR dump:
Program IR
-  type: QueryStatement
|  relation:
|  |  type: Relation
|  |  node:
|  |  |  type: LimitRelation
|  |  |  source:
|  |  |  |  type: Relation
|  |  |  |  node:
|  |  |  |  |  type: ProjectionRelation
|  |  |  |  |  source:
|  |  |  |  |  |  type: Relation
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: FileRelation
|  |  |  |  |  |  |  path: app.log
|  |  |  |  |  |  schema: [uid(1,String)]
|  |  |  |  |  projectors:
|  |  |  |  |  |  -  type: Projector
|  |  |  |  |  |  |  alias_field_id: uid(1,String)
|  |  |  |  |  |  |  expr:
|  |  |  |  |  |  |  |  type: Scalar
|  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  type: FieldScalar
|  |  |  |  |  |  |  |  |  field_id: uid(1,String)
|  |  |  |  |  |  |  |  value_type: String
|  |  |  |  |  |  -  type: Projector
|  |  |  |  |  |  |  alias_field_id: total(2,Integer)
|  |  |  |  |  |  |  expr:
|  |  |  |  |  |  |  |  type: Scalar
|  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  type: FnCallScalar
|  |  |  |  |  |  |  |  |  function:
|  |  |  |  |  |  |  |  |  |  type: Add
|  |  |  |  |  |  |  |  |  |  arg_type: Integer
|  |  |  |  |  |  |  |  |  args:
|  |  |  |  |  |  |  |  |  |  -  type: Scalar
|  |  |  |  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  |  |  |  type: ValueScalar
|  |  |  |  |  |  |  |  |  |  |  |  value: 1
|  |  |  |  |  |  |  |  |  |  |  value_type: Integer
|  |  |  |  |  |  |  |  |  |  -  type: Scalar
|  |  |  |  |  |  |  |  |  |  |  node:
|  |  |  |  |  |  |  |  |  |  |  |  type: ValueScalar
|  |  |  |  |  |  |  |  |  |  |  |  value: 2
|  |  |  |  |  |  |  |  |  |  |  value_type: Integer
|  |  |  |  |  |  |  |  value_type: Integer
|  |  |  |  schema: [uid(1,String),total(2,Integer)]
|  |  |  limit: 1
|  |  schema: [uid(1,String),total(2,Integer)]
```

</details>

## Optimizer Report: See What Changed

```sh
./lsql --print-optimize-report query.sql
```

```text
Optimizer Report
-  pass 0
|  -  folded constant scalar function call
|  -  limit(projection) -> projection(limit)
-  pass 1
```

Pass 0 folds `1 + 2` into a constant and moves the limit below the projection.
The empty pass 1 made no changes, so optimization stopped. Report pass numbers
start at zero. The maximum number of passes is controlled by
`-o Optimizer.Passes=N`.

## Optimized IR: Check Rewrites

```sh
./lsql --print-ir-optimized query.sql
```

The `total` projector now contains a `ValueScalar` with value `3`. The outer
operation is a projection whose source is a limit, so fewer records reach the
projection. Compare these nodes with the changes listed in the optimizer report.

<details>
<summary>Example output</summary>

```text
Optimized IR dump:
Program IR
-  type: QueryStatement
|  relation:
|  |  type: Relation
|  |  node:
|  |  |  type: ProjectionRelation
|  |  |  source:
|  |  |  |  type: Relation
|  |  |  |  node:
|  |  |  |  |  type: LimitRelation
|  |  |  |  |  source:
|  |  |  |  |  |  type: Relation
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: FileRelation
|  |  |  |  |  |  |  path: app.log
|  |  |  |  |  |  schema: [uid(1,String)]
|  |  |  |  |  limit: 1
|  |  |  |  schema: [uid(1,String)]
|  |  |  projectors:
|  |  |  |  -  type: Projector
|  |  |  |  |  alias_field_id: uid(1,String)
|  |  |  |  |  expr:
|  |  |  |  |  |  type: Scalar
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: FieldScalar
|  |  |  |  |  |  |  field_id: uid(1,String)
|  |  |  |  |  |  value_type: String
|  |  |  |  -  type: Projector
|  |  |  |  |  alias_field_id: total(2,Integer)
|  |  |  |  |  expr:
|  |  |  |  |  |  type: Scalar
|  |  |  |  |  |  node:
|  |  |  |  |  |  |  type: ValueScalar
|  |  |  |  |  |  |  value: 3
|  |  |  |  |  |  value_type: Integer
|  |  schema: [uid(1,String),total(2,Integer)]
```

</details>

To inspect the IR and plan with all optimizer passes disabled:

```sh
./lsql -o Optimizer.Passes=0 --print-ir-optimized --explain query.sql
```

Here the IR retains the `Add` call and the limit remains above the projection.

## Execution Plan: Check Operators and Dependencies

```sh
./lsql --explain query.sql
```

Follow each `source` down to the `Log` operator. Records flow back up through
`Log -> Limit -> Projection`. Operator `id` values identify plan nodes;
`schema` describes their output fields, and `required_fields` lists fields
needed by phase. This query runs in phase 0, so `min_phase` and `max_phase` are
both zero.

Expressions appear as `<compiled>` in the plan. Use the IR dumps to inspect
their contents.

<details>
<summary>Example output</summary>

```text
Program
-  type: Operation
|  node:
|  |  type: Projection
|  |  source:
|  |  |  type: Operation
|  |  |  node:
|  |  |  |  type: Limit
|  |  |  |  source:
|  |  |  |  |  type: Operation
|  |  |  |  |  node:
|  |  |  |  |  |  type: Log
|  |  |  |  |  |  path: app.log
|  |  |  |  |  id: 0
|  |  |  |  |  max_phase: 0
|  |  |  |  |  min_phase: 0
|  |  |  |  |  schema: [uid(1,String)]
|  |  |  |  |  required_fields:
|  |  |  |  |  |  -  phase: 0
|  |  |  |  |  |  |  fields: [uid(1,String)]
|  |  |  |  limit: 1
|  |  |  id: 1
|  |  |  max_phase: 0
|  |  |  min_phase: 0
|  |  |  schema: [uid(1,String)]
|  |  |  required_fields:
|  |  |  |  -  phase: 0
|  |  |  |  |  fields: [uid(1,String)]
|  |  projectors:
|  |  |  -  type: ScalarProjector
|  |  |  |  field_id: uid(1,String)
|  |  |  |  expr: <compiled>
|  |  |  -  type: ScalarProjector
|  |  |  |  field_id: total(2,Integer)
|  |  |  |  expr: <compiled>
|  |  scalar_context: <compiled>
|  id: 2
|  max_phase: 0
|  min_phase: 0
|  schema: [uid(1,String),total(2,Integer)]
|  required_fields:
|  |  -  phase: 0
|  |  |  fields: [total(2,Integer),uid(1,String)]
```

</details>

## Combine Diagnostics and Execute

Capture all stages in one file:

```sh
./lsql --print-ast --print-bound --print-ir-unoptimized \
  --print-optimize-report --print-ir-optimized --explain query.sql > diagnostics.txt
```

Dumps appear in pipeline order, regardless of the order of flags on the command
line. To print a plan and execute the query:

```sh
./lsql --explain --run -f JSON query.sql
```

The plan is followed by the result:

```json
{"uid":"aaa","total":3}
```

With `--run`, diagnostic dumps and query results share stdout. `-f JSON` changes
only the result format. Logs and error messages go to stderr; add `-l info` or
`-l debug` to see more runtime logging.

For errors associated with a query source span, `--print-stacktrace` adds a
stack trace when the binary was built with stack trace support. Use a Debug or
RelWithDebInfo build; Release builds disable stack traces. Check
`diagnostics.stack_traces_enabled` in `./lsql --version`.

Profiling flags collect runtime measurements and do not suppress execution.
See [profiling options](cli.md#profiling-and-errors) and
[profiling a benchmark workload](../bench/queries/README.md#profiling-one-workload).
