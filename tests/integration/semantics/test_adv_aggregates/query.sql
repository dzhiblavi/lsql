input = input.1.txt

---------------------

SELECT
    PERCENTILE(Int(latency), 0.5) AS p50,
    1 AS test_index
FROM $input

SELECT
    PERCENTILE(Int(latency), 0.1, 0.5, 0.9) AS percentiles,
    2 AS test_index
FROM $input

SELECT
    PERCENTILE(Int(latency), 0.0, 1.0) AS edge_percentiles,
    3 AS test_index
FROM $input

SELECT
    kind,
    PERCENTILE(Int(latency), 0.5) AS p50,
    COUNT(true) AS count,
    4 AS test_index
FROM $input
GROUP BY kind
ORDER BY kind

SELECT PERCENTILE(anon, 0.0, 0.5, 1.0) AS percentiles, 5 AS test_index
FROM (7)

SELECT PERCENTILE(anon, 0.0, 0.5, 1.0) AS percentiles, 6 AS test_index
FROM (9, 1, 3, 1)

SELECT PERCENTILE(anon, 0.5) AS percentiles, 7 AS test_index
FROM (1)
WHERE false

SELECT PERCENTILE(Int(anon), 0.0, 0.5, 1.0) AS percentiles, 8 AS test_index
FROM ('bad', '2', '4')

SELECT PERCENTILE(Int(anon), 0.5) AS percentiles, 9 AS test_index
FROM ('bad', '')
