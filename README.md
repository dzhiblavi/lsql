# lsql

`lsql` is a command-line tool for running SQL-style queries directly over text
logs. Use it to filter events, count errors, aggregate fields, and join related
logs from your terminal.

It reads IMAP-style and TSKV2 key/value logs, including gzip-compressed files.
Use `JSON_VALUE` to query JSON Lines. Results can be printed as a table, JSON
Lines, CSV, or TSKV.

## Example

Save these log lines as `app.log`:

```text
[2026-May-06 12:00:01.123456] method=GET path=/api status=200
[2026-May-06 12:00:02.123456] method=POST path=/api status=500
[2026-May-06 12:00:03.123456] method=GET path=/health status=200
```

Save this query as `query.sql` to count requests by status:

```sql
SELECT status, COUNT(*) AS requests
FROM app.log
GROUP BY status
ORDER BY requests DESC
```

Run `./lsql query.sql`:

```text
status  requests
------  --------
200     2
500     1
```

## Quick Start

Download the archive for your platform from
[GitHub Releases](https://github.com/dzhiblavi/lsql/releases/latest):

- Linux x86-64: the `linux-amd64` archive.
- macOS on Apple Silicon: the `macos-arm64` archive.

Extract the archive and rename the executable to `lsql`. Place it in the same
directory as `app.log` and `query.sql` from the example, then run:

```sh
./lsql --version
./lsql query.sql
```

Use `./lsql -f JSON query.sql` to print one JSON object per result row.

## Documentation

The SQL dialect is evolving. See the language reference for supported syntax.

- [Command-line options](docs/cli.md)
- [Debugging queries: AST, IR, and execution plans](docs/debugging.md)
- [SQL language](docs/sql.md)
- [Input and output formats](docs/io.md)
- Build from source: [Linux](docs/build/linux.md),
  [macOS on Apple Silicon](docs/build/mac_silicon.md)
- [Development](docs/dev.md)
- [Release notes](RELEASE_NOTES.md)

Licensed under [MIT](LICENSE).
