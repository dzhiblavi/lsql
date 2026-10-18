# SQL Style

The SQL frontend is implemented by `lsql`. It supports a compact SQL-like
language over log files and named relations. This reference describes the
current source tree; for a released version, use the documentation at its Git tag.

```sql
input = app.log

SELECT uid, status
FROM $input
WHERE status = 'fail'
ORDER BY timestamp DESC
LIMIT 20
```

## Program Structure

A program is a sequence of statements. A statement is either a named relation or
a query relation whose output is printed.

```sql
input = app.log

errors = (
    SELECT uid, timestamp, status
    FROM $input
    WHERE status != 'ok'
)

SELECT uid, timestamp
FROM $errors
```

Named relations are referenced with `$name`.

Comments start with `--` and continue to the end of the line.

Keywords are uppercase and case-sensitive. The `def` keyword and the literals
`true`, `false`, and `null` are lowercase. Function and macro names are
case-insensitive, including `COUNT` (`count(*)`, `Count(*)`, and `COUNT(*)`).

## Expression Macros

Define expression macros with lowercase `def`, followed by a name, a parenthesized
list of comma-separated parameters, `=`, and an expression. Parentheses are required
even when there are no parameters:

```sql
def minute_s() = 60
def add(x, y) = x + y + 2
def minutes(seconds) = seconds / minute_s()

SELECT add(Int(uid), Int(request)) AS result,
       minutes(Int(duration)) AS duration_minutes
FROM app.log
```

Macro names are case-insensitive, like function names. Parameter names are
case-sensitive. Every call requires parentheses, including `minute_s()`, and
the exact number of arguments. Expressions in `SELECT` still require `AS`.
A bare name refers to a field, even when a macro has the same name.

Macros can call built-in functions and previously defined macros. Forward
references, recursion, duplicate definitions, duplicate parameters, and macro
names matching built-in functions are rejected. Fields from the calling query
must be passed as arguments; subqueries in a macro body have their own fields.

## Sources and Relations

File sources:

```sql
input = path/to/log.txt
input = path/to/log.txt.gz
input = path/to/log.txt @ 2026-06-10T12:00:00 + 3600
```

`path @ timestamp + N` reads a timestamp range beginning at the given ISO-like
timestamp and spanning `N` seconds.

Gzip-compressed inputs with `.gz` or `.gzip` suffixes can be read as full-file
sources. Timestamp ranges are not supported for compressed inputs, including
default ranges passed through `--time-from` or `--time-to`.

Command output is available through `STREAM`:

```sql
SELECT uid, status
FROM STREAM 'printf "[2026-May-06 12:00:00.000000] uid=aaa status=ok\n"'
```

`STREAM 'command'` runs the command through `/bin/sh -c` and reads records from
its stdout. Use `MATERIALIZE(STREAM 'command')` when its rows need to be stored
for reuse.

File sources are treated as having fields with any name of type `String`.
When a requested field is missing in an input log line, its value is `null`.

After a relation is bound, its fields are fixed by the query. References to
fields outside that relation are rejected during binding.

Ad-hoc values:

```sql
('aaa', 'ccc')
```

Ad-hoc values form a single column named `anon`.

Named relation references:

```sql
$input
$filtered
```

Relations can be aliased with `AS`. Their fields receive the alias prefix:

```sql
SELECT r.uid AS uid, r.status AS status
FROM $input AS r

qualified = $input AS r
SELECT r.uid
FROM $qualified
```

Use qualified names such as `r.uid` to access aliased fields. Without a projection
alias, the qualified name is also the output column name. `SELECT *` includes the
currently bound fields with their alias prefixes. Aliases preserve field types
and can be applied to files, named relations, subqueries, and command streams.

`LEFT JOIN` supports one equality between expressions computed on the two inputs:

```sql
SELECT l.uid AS uid, r.status AS status
FROM requests.log AS l
LEFT JOIN responses.log AS r ON LOWER(l.uid) = LOWER(r.uid)
```

Each expression can reference fields from its own input. The operands can be
written in either order; constants are also allowed. Aggregates must be computed
in a subquery before the join. Key types follow the usual equality rules.

The inputs must have disjoint output field IDs after the whole program is bound.
Aliases give fields distinct names; explicit projections with different output
names also allow joins without aliases. Joins can be chained or nested.

Each matching right row produces an output row, including duplicate keys. A left
row without a match produces one row with `null` in every right column. Null keys
never match, including another null key. The right input is collected in memory
before reading the left input, so the right input must be finite.

A materialized relation stores its rows in memory for reuse:

```sql
MATERIALIZE (
    SELECT uid
    FROM $input
)
```

Subqueries are written in parentheses.

```sql
SELECT uid
FROM (
    SELECT uid
    FROM $input
    WHERE kind = 'api'
)
```

## SELECT

Supported clause order:

```sql
SELECT select_list
FROM source
WHERE condition
GROUP BY group_list
HAVING condition
ORDER BY order_list [ASC|DESC]
LIMIT n
```

All clauses after `FROM` are optional.

Computed expressions in `SELECT` require an explicit `AS` alias. Plain field
references can be selected without an alias.

Projection examples:

```sql
SELECT uid, status
FROM $input

SELECT user AS username, action AS event
FROM $input

SELECT *, 1 AS test_index
FROM $input

SELECT 'literal' AS kind, 42 AS answer
FROM $input
LIMIT 1
```

## GROUP BY

Aggregates are used directly in the `SELECT` list.

```sql
SELECT status, COUNT(*) AS count
FROM $input
GROUP BY status
ORDER BY status
```

Multiple grouping keys are supported.

```sql
SELECT status, kind, COUNT(*) AS count
FROM $input
GROUP BY status, kind
ORDER BY count, kind DESC
```

`HAVING` filters grouped rows after aggregation. In `lsql`, `HAVING` is only
valid together with `GROUP BY`. It can reference grouping keys, aggregate
expressions, and `SELECT` aliases.

```sql
SELECT status, COUNT(*) AS count
FROM $input
GROUP BY status
HAVING count > 10
ORDER BY count DESC
```

## ORDER BY and LIMIT

```sql
SELECT timestamp, uid
FROM $input
ORDER BY timestamp

SELECT timestamp, uid
FROM $input
ORDER BY timestamp DESC
LIMIT 2
```

`ASC` is accepted and is the default. A trailing `ASC` or `DESC` applies to
all expressions in the sort key list. For example, `ORDER BY count, kind DESC`
sorts both keys in descending order.

## UNION

Plain concatenation:

```sql
$input1
UNION ALL
$input2
```

Merge two sorted relations by one or more expressions:

```sql
SELECT *
FROM $input1
ORDER BY timestamp

UNION ALL SORTED BY timestamp

SELECT *
FROM $input2
ORDER BY timestamp
```

`UNION ALL SORTED BY ... DESC` is also supported.

## Fields

Log fields are referenced as bare identifiers:

```sql
uid
status
timestamp
```

Missing fields from file sources evaluate to `null`. References to fields that
are not present in a bound relation are rejected during binding.

`*` projects all visible fields. For files, field references in the query
define those fields; `SELECT *` does not discover every key present in the log.

The full raw input line is available as `_line`.

## Expressions

Literals:

```sql
'string'
42
3.14
true
false
null
```

Operators:

```sql
a = b
a != b
a < b
a <= b
a > b
a >= b
a AND b
a OR b
NOT a
!a
a + b
a - b
a / b
a LIKE 'regex'
a IN source
```

Arithmetic requires compatible types. Cast string log fields with `Int(...)`
or `Float(...)` before numeric operations. Integer division truncates the
result; division by zero returns `null`. When a typed operand evaluates to
`null`, for example `Int('bad')`, arithmetic also returns `null`. String `+`
concatenates its operands.

Equality allows comparing with `null`: `null = null` is `true`, and a non-null
value is unequal to `null`. Use `field = null` or `field != null` to test for
missing values.

`LIKE` matches a regular expression against the whole string. For a substring
match, include `.*` around the pattern:

```sql
SELECT anon, anon LIKE 'error' AS exact_match,
       anon LIKE '.*error.*' AS contains_match
FROM ('error', 'prefix error suffix')
```

Both rows match `.*error.*`; only the first matches `error`. The pattern must be
a string literal. SQL wildcard patterns such as `%error%` are not used.

## Functions

Casts:

```sql
String(expr)
Int(expr)
Float(expr)
Bool(expr)
Null(expr)
```

Casts preserve `null`; `Null(expr)` always returns `null`. Invalid numeric string
conversions return `null`. `Int(...)` requires a complete decimal integer string
with an optional leading `-`; a leading `+` is not accepted. `Float(...)`
requires a complete finite number. Leading or trailing whitespace is not
accepted.

Casting a string with `Bool(...)` tests whether it is non-empty: `Bool('')` is
`false`, while `Bool('false')` and `Bool('0')` are both `true`.

JSON value extraction:

```sql
JSON_VALUE(json, 'messages[0].title')
JSON_VALUE(json, '$.messages[0].title')
JSON_VALUE(json, '/messages/0/title')
```

The path must be a string literal. Dot-separated object fields and bracketed
array indexes are supported, with an optional `$` prefix. RFC 6901 JSON Pointer
paths are also accepted.

`JSON_VALUE` returns a nullable string. JSON strings are decoded, while numbers,
booleans, objects, and arrays retain their JSON representation. Missing paths,
malformed JSON, and JSON `null` produce `null`. Use a cast such as
`Int(JSON_VALUE(json, 'count'))` when a typed result is needed.

JSON Lines sources expose root keys directly as nullable string fields;
use `JSON_VALUE` on an object or array field for nested access. Queries that
only require `_line` can also process arbitrary line-oriented input.

Other scalar functions:

| Function | Behavior |
| --- | --- |
| `LOWER(expr)` | Convert a string to lowercase. |
| `SPLITPART(expr, 'separator', index)` | Select a zero-based part; return `null` if the part does not exist. The separator must be a one-character string literal and the index a non-negative integer literal. |
| `SUBSTR(expr, start)` | Take a substring from a zero-based offset to the end. |
| `SUBSTR(expr, start, length)` | Take a substring with the given length. Offsets and lengths must be non-negative integer literals. |
| `COALESCE(expr, ...)` | Return the first non-null argument; all non-null argument types must agree. |
| `RSUBSTR(expr, 'regex')` | Return the first matching substring, or `null` if there is no match. The regex must be a string literal. |
| `PARSE_TIMESTAMP(expr, 'format')` | Parse a timestamp string into integer seconds since the Unix epoch, using the local timezone. |

For example:

```sql
SELECT LOWER(anon) AS lowered,
       SPLITPART(anon, ':', 1) AS part,
       SUBSTR(anon, 0, 3) AS prefix,
       SUBSTR(anon, 4) AS suffix
FROM ('ABC:def')
```

`PARSE_TIMESTAMP` requires a format string literal with one of these exact names:

| Format | Example input |
| --- | --- |
| `SQL` | `2026-05-06 12:00:00` |
| `ORACLE` | `2026-May-06 12:00:00` |
| `ISO8601` | `2026-05-06T12:00:00` |

```sql
SELECT PARSE_TIMESTAMP('2026-May-06 12:00:01', 'ORACLE')
       - PARSE_TIMESTAMP('2026-05-06T12:00:00', 'ISO8601') AS elapsed_seconds
FROM (1)
```

Aggregates:

```sql
COUNT(*)
COUNT(expr)
MIN(expr)
MAX(expr)
SUM(expr)
PERCENTILE(expr, p, ...)
```

`COUNT(*)` counts all rows; `COUNT(expr)` counts non-null values. The functions
`COUNT_ALL()` and `COUNT_NONNULL(expr)` are also accepted as equivalents.

`PERCENTILE` takes one or more floating-point literals between `0.0` and `1.0`
for its percentile arguments. It returns a string containing a bracketed list,
even for a single percentile, such as `'[25]'`. Null input values are ignored;
an empty input produces `null`.

## IN and Semi-Joins

Literal membership:

```sql
SELECT uid
FROM $input
WHERE uid IN ('aaa', 'ccc')
```

Named relation membership:

```sql
uids = (
    SELECT uid
    FROM $input
    WHERE status = 'ok'
)

SELECT uid
FROM $input
WHERE uid IN $uids
```

Subquery membership:

```sql
SELECT uid
FROM $input
WHERE uid IN (
    SELECT uid
    FROM $input
    WHERE kind = 'worker'
)
```

Negated membership, using `$uids` from the previous example:

```sql
SELECT uid
FROM $input
WHERE NOT uid IN $uids
```

## Examples

Find failing request groups with the highest latency:

```sql
input = app.log

SELECT
    status,
    COUNT(*) AS count,
    MAX(Int(latency)) AS max_latency
FROM $input
WHERE status != 'ok'
GROUP BY status
ORDER BY max_latency DESC
LIMIT 5
```

Extract a regex match:

```sql
input = app.log

SELECT
    uid,
    COALESCE(RSUBSTR(request, 'user=[\d]+'), 'missing') AS user_id
FROM $input
```

Use a named relation:

```sql
input = app.log

bad_uids = (
    SELECT uid
    FROM $input
    WHERE status = 'fail'
)

SELECT uid, path, status
FROM $input
WHERE uid IN $bad_uids
```
