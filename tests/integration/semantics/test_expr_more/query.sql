input = input.1.txt

---------------------

SELECT
    uid,
    Bool(flag) AS flag_b,
    Bool(missing) AS missing_b,
    Int(latency) / Int(divisor) AS ratio,
    1 AS test_index
FROM $input
ORDER BY uid

SELECT
    SUM(Int(latency)) AS total_latency,
    SUM(Int(divisor)) AS total_divisor,
    2 AS test_index
FROM $input
