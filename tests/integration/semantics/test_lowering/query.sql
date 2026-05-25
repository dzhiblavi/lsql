input = input.1.txt

---------------------

SELECT
    MIN(Int(latency)) + MAX(Int(latency)) AS span,
    1 AS test_index
FROM $input

SELECT
    COALESCE(tag, 'unknown') AS tag_value,
    2 AS test_index
FROM $input

SELECT
    uid,
    NOT status = 'ok' AS bad,
    3 AS test_index
FROM $input

SELECT
    uid,
    status = 'ok' OR status = 'warn' AS acceptable,
    4 AS test_index
FROM $input

SELECT
    MIN(Int(latency)) AS min_latency,
    MAX(Int(latency)) AS max_latency,
    MAX(Int(latency)) - MIN(Int(latency)) AS delta,
    5 AS test_index
FROM $input
