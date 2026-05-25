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
