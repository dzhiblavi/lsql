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

SELECT anon AS text, Int(anon) / Int('0') AS ratio, 3 AS test_index
FROM ('1', '0', 'bad')

SELECT
    anon AS text,
    Int(anon) + 1 AS incremented,
    10 - Int(anon) AS difference,
    (Int(anon) = null) AS missing,
    4 AS test_index
FROM ('2', '3')

SELECT
    anon AS flag,
    anon AND true AS conjunction,
    anon OR false AS disjunction,
    NOT anon AS negated,
    NOT NOT anon AS double_negation,
    true OR false AND false AS precedence,
    5 AS test_index
FROM (true, false)
