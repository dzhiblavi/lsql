# Command-Line Options

```sh
./lsql [options] [query.sql]
```

The positional argument is the query file. When it is omitted, the query is read
from standard input:

```sh
./lsql < query.sql
```

For locally built binaries, use `./output/lsql-Debug` or
`./output/lsql-Release` instead of `./lsql`.

## Execution and Output

| Option | Description |
| --- | --- |
| `-h`, `--help` | Show command-line help. |
| `--version` | Show version, build information, and available dynamic settings with their defaults. |
| `-f`, `--format FORMAT` | Output format: `JSON`, `TSKV`, `CSVHeader`, or `PrettyTable` (default). |
| `-l`, `--log-level LEVEL` | Logging level: `trace`, `debug`, `info`, `warn` (default), `err`, `critical`, or `off`. |
| `-j`, `--threads N` | Maximum number of worker threads; default `1`. |
| `-k`, `--keep-output-order` | Buffer output groups and print them in query order. |
| `--no-buffer` | Disable per-query output buffering. |
| `--time-from TIMESTAMP` | Default lower input timestamp, in ISO8601 format. |
| `--time-to TIMESTAMP` | Default upper input timestamp, in ISO8601 format. |
| `-o`, `--option GROUP.KEY=VALUE` | Override an integer dynamic setting; can be repeated. |

The optimizer runs five passes by default. For example, select JSON output and
set the number of optimization passes explicitly:

```sh
./lsql -f JSON -o Optimizer.Passes=5 query.sql
```

For output ordering, buffering, and compressed input restrictions, see
[Input and Output](io.md).

## Query Diagnostics

See [Debugging Queries](debugging.md) for examples of every dump and how to read
them.

| Option | Description |
| --- | --- |
| `--explain` | Print the execution plan. |
| `--print-ast` | Print the frontend AST. |
| `--print-bound` | Print the bound AST. |
| `--print-ir-unoptimized` | Print IR before optimization. |
| `--print-ir-optimized` | Print IR after optimization. |
| `--print-optimize-report` | Print the optimization report. |
| `--run` | Execute the query when query diagnostic flags are used. |

Queries are not executed when `--explain` or any of the AST, IR, or optimization
report flags above are used. Add `--run` to also execute the query:

```sh
./lsql --explain query.sql
./lsql --explain --run query.sql
```

## Profiling and Errors

| Option | Description |
| --- | --- |
| `--profile-text` | Print per-phase text profiles to stderr. |
| `--profile-flamegraph` | Write `prof.<N>.folded` files. |
| `--profile-dot-graph` | Write `prof.dot`. |
| `--print-stacktrace` | Print a stack trace for errors associated with a query source span. |
