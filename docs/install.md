# Install and Run lsql

## Download and Install

Download the `.tar.gz` archive for your platform from
[GitHub Releases](https://github.com/dzhiblavi/lsql/releases/latest), save it as
`lsql.tar.gz`, and open a terminal in the directory containing the download.

| Platform | Archive name contains |
| --- | --- |
| Linux x86-64 with glibc | `linux-amd64` |
| macOS 15 or later on Apple Silicon | `macos-arm64` |

### Linux x86-64

```sh
tar -xzf lsql.tar.gz
mkdir -p "$HOME/.local/bin"
install -m 755 lsql-Release-linux-amd64 "$HOME/.local/bin/lsql"
```

### macOS on Apple Silicon

```sh
tar -xzf lsql.tar.gz
mkdir -p "$HOME/.local/bin"
install -m 755 lsql-Release-macos-arm64 "$HOME/.local/bin/lsql"
```

To build from source, follow the [Linux](build/linux.md) or
[macOS](build/mac_silicon.md) build instructions.

## Add lsql to PATH

Make the installed command available in the current terminal:

```sh
export PATH="$HOME/.local/bin:$PATH"
lsql --version
```

For future terminals, add the same `export PATH` line to your shell configuration
file: `~/.zshrc` for zsh or `~/.bashrc` for bash.

## Run Your First Query

Create a directory with a small log file and a query:

```sh
mkdir lsql-example
cd lsql-example
cat > app.log <<'EOF'
[2026-May-06 12:00:01.123456] method=GET path=/api status=200
[2026-May-06 12:00:02.123456] method=POST path=/api status=500
[2026-May-06 12:00:03.123456] method=GET path=/health status=200
EOF
cat > query.sql <<'EOF'
SELECT status, COUNT(*) AS requests
FROM app.log
GROUP BY status
ORDER BY requests DESC
EOF
lsql query.sql
```

Expected output:

```text
status  requests
------  --------
200     2
500     1
```

For JSON Lines output:

```sh
lsql -f JSON query.sql
```

```json
{"status":"200","requests":2}
{"status":"500","requests":1}
```

Continue with the [SQL reference](sql.md), [CLI options](cli.md), or
[query debugging guide](debugging.md).
