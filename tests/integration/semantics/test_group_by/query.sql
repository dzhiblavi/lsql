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
