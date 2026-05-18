SELECT
    a,
    COUNT(true) AS count,
    MIN(Int(b)) AS min_b,
    MAX(Int(b)) AS max_b
FROM input.txt
GROUP BY a
