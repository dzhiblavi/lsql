# logsql

`logsql` is a small query engine for text logs. It reads log files as streams of
records, extracts key/value fields, and runs query plans over them.

The project currently exposes two command-line tools over the same execution
engine:

- `lsql`: SQL style.
- `lpipe`: unix pipe style.

The two languages are mostly functionally equivalent, so queries can be written
in whichever style is clearer for the task.

## Status

The language should be treated as evolving. The current behavior is best
described by the integration tests and the language docs.

## Quick Example

Given an IMAP-style log file:

```text
[2026-May-06 12:00:01.123456] uid=aaa user=alice status=ok action=login
[2026-May-06 12:00:02.654321] uid=bbb user=bob status=fail action=logout
```

SQL style:

```sql
input = input.1.txt

SELECT uid, status
FROM $input
WHERE status = 'ok'
```

Unix pipe style:

```text
input = file input.1.txt

$input
| where .status = 'ok'
| select .uid, .status
```

Run them with:

```sh
./output/lsql-Debug -f JSON query.sql
./output/lpipe-Debug -f JSON query.pipe
```

## Command-Line Options

Both `lsql` and `lpipe` share the same CLI flags.

```text
<path>                    query file; stdin is used when omitted
-f, --format              JSON, TSKV, CSVHeader, PrettyTable; default PrettyTable
-l, --log-level           Trace, Debug, Info, Warn, Err, Critical, Off
-j, --threads             max worker threads
-k, --keep-output-order   buffer output groups and print them in query order
--time-from               default lower timestamp, ISO8601
--time-to                 default upper timestamp, ISO8601
-o, --optimize-passes     number of optimization passes; default 5
--explain                 print execution plan
--run                     force execution when using diagnostic flags
--print-ast               print frontend AST
--print-bound             print bound AST
--print-ir-unoptimized    print IR before optimization
--print-ir-optimized      print IR after optimization
--print-optimize-report   print optimization report
--profile-text            print per-phase text profile to stderr
--profile-flamegraph      write prof.<N>.folded files
--profile-dot-graph       write prof.dot
--print-stacktrace        print stacktrace on span errors
```

When diagnostic flags such as `--print-ast` or `--explain` are used, the query is
not executed unless `--run` is also passed.

## Query Languages

See:

- [SQL style](docs/sql.md)
- [Unix pipe style](docs/pipe.md)

The two languages are intended to cover the same core operations. Choose SQL
style or unix pipe style by preference and by the shape of the query.

Build and development docs:

- [Build](docs/build.md)
- [Development](docs/dev.md)

## Log Input

The backend detects the log format from the beginning of the file. Current
formats are:

- `IMAP`: lines beginning with `[timestamp]`, followed by space-separated
  `key=value` fields. The timestamp is exposed as `timestamp`.
- `TSKV2`: tab-separated tokens where `key=value` tokens become fields.

Tokens without `=` may be exposed through anonymous column names. The full raw
line is available through `_line`.

File sources are treated as having fields with any name of type `String`. When a
requested field is missing in an input log line, its value is `null`.

After a relation is bound, its fields are fixed by the query. References to
fields outside that relation are rejected during binding.

Gzip-compressed inputs with `.gz` or `.gzip` suffixes are supported for full-file
scans. Timestamp ranges are not supported for compressed inputs, including
explicit query ranges and default ranges passed through `--time-from` or
`--time-to`.

## Output

Supported output formats:

- `JSON`: one JSON object per output record.
- `TSKV`: tab-separated `field=value` pairs.
- `CSVHeader`: a header row followed by comma-separated rows.
- `PrettyTable`: a buffered, human-readable table with aligned columns.

Multiple top-level queries produce multiple output groups. The order of those
groups is unspecified by default. Use `--keep-output-order` to buffer output
groups and print them in query order.
