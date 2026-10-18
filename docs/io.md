# Input and Output

## Input

The format is detected automatically. Each record occupies one line.

| Format | Record | Fields |
| --- | --- | --- |
| Timestamped logfmt (`TS_LOGFMT`) | `[timestamp] key=value ...` | Named fields and `timestamp` |
| TSKV | Tab-separated `key=value` pairs with a `tskv` marker | Named fields |
| CSV | Comma-separated values | `anon1` through `anon32` |
| TSV | Tab-separated values | `anon1` through `anon32` |
| JSON Lines (`JSONL`) | One JSON object | Root keys |

Values are strings; missing fields and JSON `null` become `null`. Use explicit
casts for numbers. The original line is available as `_line`. In key/value
formats, tokens without a key become `anon1` through `anon32`.

CSV supports quoted fields and doubled quotes. TSV preserves quotes literally.
Empty fields remain empty strings. CSV and TSV have no header handling.

JSON strings and keys are unescaped. Numbers and booleans retain their text;
objects and arrays remain JSON text, accessible with [`JSON_VALUE`](sql.md#functions).
For repeated JSON keys, the last value wins.

Gzip files (`.gz`, `.gzip`) are supported. Time ranges require uncompressed,
timestamp-sorted logfmt or TSKV input; CSV, TSV, and JSONL do not support them.

`STREAM 'command'` reads records from the command's stdout in the same input
formats. For example, use `FROM STREAM 'cat requests.jsonl'` to read command
output instead of a file directly.

## Example

Save this as `requests.jsonl`:

```json
{"status":500,"meta":{"path":"/api"}}
```

Save this as `query.sql`:

```sql
SELECT Int(status) AS status, JSON_VALUE(meta, 'path') AS path
FROM requests.jsonl
```

Run `./lsql -f JSON query.sql`:

```json
{"status":500,"path":"/api"}
```

## Output

- `JSON`: one JSON object per row.
- `TSKV`: tab-separated `field=value` pairs.
- `CSVHeader`: a header followed by comma-separated rows, without quoting or escaping.
- `PrettyTable`: a buffered table with aligned columns.

See [CLI options](cli.md) for output ordering and buffering.
