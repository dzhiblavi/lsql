input = input.1.txt

---------------------

SELECT status, COUNT(*) AS count, 1 AS test_index
FROM $input
GROUP BY status
ORDER BY status

SELECT status, kind, COUNT(*) AS count, 2 AS test_index
FROM $input
GROUP BY status, kind
ORDER BY count, kind DESC

SELECT status AS state, COUNT(*) AS count, 3 AS test_index
FROM $input
GROUP BY status
ORDER BY count DESC

SELECT COUNT(*) AS count, 4 AS test_index
FROM $input
GROUP BY uid
ORDER BY uid

SELECT
    COUNT(*) AS count,
    5 AS test_index,
    'user_' + uid AS user_label
FROM $input
GROUP BY uid
ORDER BY uid

SELECT
    status AS state,
    COUNT(*) AS count,
    6 AS test_index
FROM $input
GROUP BY status
ORDER BY status

SELECT
    kind,
    COUNT(*) AS count,
    7 AS test_index
FROM $input
GROUP BY kind, region
ORDER BY region, kind

SELECT
    region,
    MAX(Int(latency)) AS max_latency,
    8 AS test_index
FROM $input
GROUP BY region
ORDER BY max_latency DESC

SELECT status, COUNT(*) AS cnt, 9 AS test_index
FROM $input
GROUP BY status
HAVING cnt >= 3
ORDER BY status

SELECT status, 10 AS test_index
FROM $input
GROUP BY status
HAVING COUNT(*) > 3
ORDER BY status

SELECT key, COUNT(*) AS rows, COUNT(Int(anon)) AS values, 11 AS test_index
FROM ('1', '01', 'bad', 'bad', '2')
GROUP BY Int(anon) AS key
ORDER BY key

SELECT key, COUNT(*) AS rows, 12 AS test_index
FROM ('A', 'a', 'B')
GROUP BY LOWER(anon) AS key
ORDER BY key

SELECT COUNT(*) AS rows, 13 AS test_index
FROM (SELECT anon, COUNT(*) AS count FROM (1) WHERE false GROUP BY anon)

SELECT
    anon AS key,
    COUNT(*) AS rows,
    COUNT(*) + 1 AS adjusted,
    COUNT(Int(anon)) AS values,
    SUM(Int(anon)) AS total,
    14 AS test_index
FROM ('1', '1', '2', 'bad', 'bad')
GROUP BY anon
HAVING COUNT(*) > 1 AND (SUM(Int(anon)) >= 2 OR COUNT(Int(anon)) = 0)
ORDER BY key
