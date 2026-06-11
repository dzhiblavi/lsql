# SQL Style

The SQL frontend is implemented by `lsql`. It supports a compact SQL-like
language over log files and named relations.

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

Keywords are uppercase and currently case-sensitive, except literals such as
`true`, `false`, and `null`, and casts such as `Int(...)`.

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

File sources are treated as having fields with any name of type `String`.
When a requested field is missing in an input log line, its value is `null`.

After a relation is bound, its fields are fixed by the query. References to
fields outside that relation are rejected during binding.

Ad-hoc values:

```sql
('aaa', 'ccc')
```

Named relation references:

```sql
$input
$filtered
```

Materialized relation:

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
ORDER BY order_list [ASC|DESC]
LIMIT n
```

All clauses after `FROM` are optional.

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

`ASC` is accepted and is the default.

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

`*` projects all visible fields.

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

Arithmetic requires compatible types. Equality allows comparing with `null`.

## Functions

Casts:

```sql
String(expr)
Int(expr)
Float(expr)
Bool(expr)
```

Other scalar functions:

```sql
COALESCE(expr, ...)
RSUBSTR(expr, 'regex')
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

`PERCENTILE` returns a string representation of the selected percentile value or
values.

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

Negated membership:

```sql
SELECT uid
FROM $input
WHERE NOT uid IN $uids
```

## Examples

Count slow failing requests:

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
