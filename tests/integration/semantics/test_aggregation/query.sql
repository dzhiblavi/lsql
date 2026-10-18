input = input.1.txt

---------------------

SELECT COUNT(*) AS count, 1 AS test_index
FROM $input

SELECT COUNT(true) AS count_true, COUNT(false) AS count_false, COUNT(null) AS count_null, 2 AS test_index
FROM $input

SELECT MIN(timestamp) AS min_ts, MAX(timestamp) AS max_ts, 3 AS test_index
FROM $input

SELECT MIN(uid) AS min_uid, MAX(uid) AS max_uid, 4 AS test_index
FROM $input

SELECT COUNT(*) AS count, MIN(timestamp) AS min_ts, MAX(timestamp) AS max_ts, 5 AS test_index
FROM $input
WHERE status = 'ok'

SELECT
    COUNT(*) AS rows,
    COUNT(Int(anon)) AS values,
    MIN(Int(anon)) AS minimum,
    MAX(Int(anon)) AS maximum,
    SUM(Int(anon)) AS total,
    6 AS test_index
FROM (1)
WHERE false

SELECT
    COUNT(*) AS rows,
    COUNT(Int(anon)) AS values,
    MIN(Int(anon)) AS minimum,
    MAX(Int(anon)) AS maximum,
    SUM(Int(anon)) AS total,
    7 AS test_index
FROM ('bad', '')

SELECT
    COUNT(*) AS rows,
    COUNT(Int(anon)) AS values,
    MIN(Int(anon)) AS minimum,
    MAX(Int(anon)) AS maximum,
    SUM(Int(anon)) AS total,
    8 AS test_index
FROM ('3', 'bad', '-2', '3')

SELECT
    SUM(Int(anon)) AS total,
    SUM(Int(anon)) + 1 AS adjusted,
    SUM(Int(anon) + 1) AS shifted,
    COUNT(Int(anon)) AS values,
    COUNT(*) AS rows,
    9 AS test_index
FROM ('3', 'bad', '-2', '3')

SELECT
    COUNT(*) AS rows,
    COUNT(*) + 1 AS adjusted,
    SUM(Int(anon)) AS total,
    SUM(Int(anon)) + 1 AS next_total,
    MIN(5) AS minimum,
    MIN(5) AS same_minimum,
    10 AS test_index
FROM (1)
WHERE false
