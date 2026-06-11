# Unix Pipe Style

The unix pipe style is implemented by `lpipe`. It is line-oriented in style but
not newline-sensitive: whitespace is generally ignored.

```text
input = file app.log

$input
| where .status = 'fail'
| select .timestamp, .uid, .path
| sort by .timestamp desc
| take 20
```

## Files and Pipelines

A program is a sequence of statements. A statement is either a named pipeline or
a query pipeline whose output is printed.

```text
input = file app.log

errors =
    $input
    | where .status = 'fail'

$errors
| select .timestamp, .uid
```

Named pipelines are referenced with `$name`.

Comments start with `--` and continue to the end of the line.

Keywords are lowercase and currently case-sensitive.

## Sources

```text
file path/to/log.txt
file path/to/log.txt.gz
file path/to/log.txt @ 2026-06-10T12:00:00 + 3600
values ('aaa', 'ccc')
$name
union (pipeline) (pipeline)
merge by .timestamp (pipeline) (pipeline)
merge by .timestamp desc (pipeline) (pipeline)
```

`file ... @ ... + N` reads a timestamp range beginning at the given ISO-like
timestamp and spanning `N` seconds.

Gzip-compressed inputs with `.gz` or `.gzip` suffixes can be read as full-file
sources. Timestamp ranges are not supported for compressed inputs, including
default ranges passed through `--time-from` or `--time-to`.

File sources are treated as having fields with any name of type `String`.
When a requested field is missing in an input log line, its value is `null`.

After a relation is bound, its fields are fixed by the query. References to
fields outside that relation are rejected during binding.

`values (...)` creates an ad-hoc one-column source, mainly useful with `in`.

`union` concatenates two pipelines. `merge by` merges two already sorted
pipelines by one or more expressions.

## Stages

### `where` / `filter`

Filters records by a boolean expression.

```text
$input
| where .status = 'ok' and .uid = 'aaa'
```

`filter` is an alias for `where`.

### `select`

Projects fields and expressions.

```text
$input
| select .uid, .status

$input
| select
    .user as username,
    int(.latency) as latency_ms,
    'api' as kind
```

`*` keeps all visible fields.

```text
$input
| select *, 1 as test_index
```

### Grouped `select`

Aggregates are expressed as a `select ... group by ...` stage.

```text
$input
| select
    .status,
    count(*) as count,
    max(int(.latency)) as max_latency
  group by .status
```

Grouping fields that must be used later can be selected, aliased, or projected
again in a later `select`.

### `sort by`

Sorts records by one or more expressions.

```text
$input | sort by .timestamp
$input | sort by .timestamp desc
$input | sort by .count, .kind desc
```

`asc` is accepted and is the default.

### `take` / `limit`

Limits the number of records.

```text
$input | take 10
$input | limit 10
```

`limit` is an alias for `take`.

## Fields

Fields are written as identifiers. Log fields are normally referenced with a
leading dot:

```text
.uid
.status
.timestamp
```

The lexer also accepts bare identifiers in expression positions, but the tests
and examples consistently use dotted field references for log fields.

Missing fields from file sources evaluate to `null`. References to fields that
are not present in a bound relation are rejected during binding.

The full raw input line is available as `._line`.

## Expressions

Literals:

```text
'string'
42
3.14
true
false
null
```

Operators:

```text
a = b
a != b
a and b
a or b
not a
!a
a + b
a - b
a / b
a like 'regex'
a in (pipeline)
```

The `like` and `rsubstr` patterns are regular expressions.

Arithmetic requires compatible types. Equality allows comparing with `null`.

## Functions

Casts:

```text
string(expr)
int(expr)
float(expr)
bool(expr)
```

Other scalar functions:

```text
coalesce(expr, ...)
rsubstr(expr, 'regex')
```

Aggregates:

```text
count(*)
count(expr)
min(expr)
max(expr)
sum(expr)
percentile(expr, p, ...)
```

`percentile` returns a string representation of the selected percentile value or
values.

## Semi-Joins

Use `in (pipeline)` to keep records whose expression appears in another
pipeline.

```text
uids =
    $input
    | where .status = 'ok'
    | select .uid

$input
| where .uid in (
    $uids
)
| select .uid
```

Literal membership can be expressed with `values`.

```text
$input
| where .uid in (
    values ('aaa', 'ccc')
)
| select .uid
```

## Examples

Count failures by status:

```text
input = file app.log

$input
| where .status != 'ok'
| select .status, count(*) as count group by .status
| sort by .count desc
```

Extract a regex match from a request field:

```text
input = file app.log

$input
| select
    .uid,
    coalesce(rsubstr(.request, 'user=[\d]+'), 'missing') as user_id
```

Merge two sorted logs:

```text
left = file app.1.log
right = file app.2.log

merge by .timestamp (
    $left | sort by .timestamp
) (
    $right | sort by .timestamp
)
| select .timestamp, .uid, .status
```
