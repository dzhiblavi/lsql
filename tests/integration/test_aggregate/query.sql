SELECT
    COUNT(true) AS count,
    MIN(Int(anon1)) AS min,
    MAX(Int(anon1)) AS max,
    SUM(Int(anon1)) AS sum
FROM input.txt
