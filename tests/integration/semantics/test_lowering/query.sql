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

SELECT anon AS text, COALESCE(anon, 'fallback') AS value, 6 AS test_index
FROM ('', 'present')

SELECT anon AS text, COALESCE(Int(anon), 0, 99) AS value, 7 AS test_index
FROM ('bad', '0', '1')

SELECT anon AS flag, COALESCE(anon, true) AS value, 8 AS test_index
FROM (false, true)
