# Input and Output

## Log Input

The backend detects the log format from the beginning of the file. Current
formats are:

- `IMAP`: lines beginning with `[timestamp]`, followed by space-separated
  `key=value` fields. The timestamp is exposed as `timestamp`. Detection looks
  for a line beginning with `[`.
- `TSKV2`: tab-separated tokens where `key=value` tokens become fields.
  Detection looks for a line containing `tskv`, so include the `tskv` marker
  in tab-separated key/value logs.

For example, save these IMAP-style lines as `input.1.txt`:

```text
[2026-May-06 12:00:01.123456] uid=aaa user=alice status=ok action=login
[2026-May-06 12:00:02.654321] uid=bbb user=bob status=fail action=logout
```

Save this query as `query.sql`:

```sql
input = input.1.txt

SELECT uid, status
FROM $input
WHERE status = 'ok'
```

Run `./lsql -f JSON query.sql` to print the matching records as JSON.

Tokens without `=` are exposed as `anon1`, `anon2`, up to `anon32`, in order.
The full raw line is available through `_line`. Ad-hoc value relations use a
single column named `anon`, as described in the [SQL reference](sql.md#sources-and-relations).

For JSON Lines and other line-oriented input, read `_line` and extract fields
with [`JSON_VALUE`](sql.md#functions).

File sources are treated as having fields with any name of type `String`. When a
requested field is missing in an input log line, its value is `null`.

After a relation is bound, its fields are fixed by the query. References to
fields outside that relation are rejected during binding.

File timestamp ranges require logs in ascending timestamp order.

Gzip-compressed inputs with `.gz` or `.gzip` suffixes are supported for full-file
scans. Timestamp ranges are not supported for compressed inputs, including
explicit query ranges and default ranges passed through `--time-from` or
`--time-to`.

## Output

Supported output formats:

- `JSON`: one JSON object per output record.
- `TSKV`: tab-separated `field=value` pairs.
- `CSVHeader`: a header row followed by comma-separated rows. Values are
  written without CSV quoting or escaping.
- `PrettyTable`: a buffered, human-readable table with aligned columns.

Multiple top-level queries produce multiple output groups. The order of those
groups is unspecified by default. Use `--keep-output-order` to buffer output
groups and print them in query order.

Use `--no-buffer` to avoid buffering the complete result of a single top-level
query. It requires exactly one top-level query and supports JSON, TSKV, and
CSVHeader; it cannot be combined with `--keep-output-order` or PrettyTable. Standard stdout buffering is retained.
