SELECT anon, COUNT(*) AS count
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (2)
ORDER BY anon

SELECT anon, COUNT(*) AS count, 2 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (2, 2) AND anon = 1
ORDER BY anon

SELECT anon, COUNT(*) AS count, 3 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING NOT count IN (2)
ORDER BY anon

SELECT anon, COUNT(*) AS count, 4 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (SELECT anon FROM (1, 2) WHERE anon = 2)
ORDER BY anon

SELECT COUNT(*) AS count, 5 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING anon IN (1)
ORDER BY anon

SELECT COUNT(key) AS n, 6 AS test_index
FROM ('1', '01', 'bad', '2')
GROUP BY Int(anon) AS key
HAVING key IN (SELECT Int(anon) AS value FROM ('bad', '1'))
ORDER BY key

SELECT COUNT(*) AS rows, 7 AS test_index
FROM (
    SELECT anon, COUNT(*) AS count
    FROM (1, 1, 2)
    GROUP BY anon
    HAVING count IN (SELECT anon FROM (2) WHERE false)
)

SELECT COUNT(*) AS rows, 8 AS test_index
FROM (
    SELECT anon, COUNT(*) AS count
    FROM (1, 2)
    WHERE false
    GROUP BY anon
    HAVING count IN (1)
)

SELECT anon, COUNT(*) AS count, 9 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (2) OR anon IN (2)
ORDER BY anon

SELECT anon, anon = 1 AS selected, COUNT(*) AS n, 10 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING selected IN (true)
ORDER BY anon

SELECT anon, COUNT(*) + 1 AS size, 11 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING size IN (3)
ORDER BY anon

SELECT 'id=' + String(anon) AS label, COUNT(*) AS n, 12 AS test_index
FROM (3, 3, 1, 1, 2)
GROUP BY anon
HAVING n IN (2)
ORDER BY anon DESC
LIMIT 1

SELECT anon, 13 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING COUNT(*) + 1 > 2
ORDER BY anon

SELECT anon, 14 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING String(COUNT(*)) = '2'
ORDER BY anon

SELECT anon, 15 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING COUNT(*) > 1 AND SUM(anon) = 2
ORDER BY anon

SELECT anon, 16 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING COUNT(anon IN (1)) > 1
ORDER BY anon

SELECT anon, COUNT(*) AS count, 17 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (SELECT COUNT(*) AS size FROM (9, 9))
ORDER BY anon

SELECT anon, COUNT(anon) AS n, 18 AS test_index
FROM (1, 1, 2)
GROUP BY anon + 1 AS anon
HAVING anon IN (2)
ORDER BY anon

SELECT anon, COUNT(*) AS count, 19 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING (count + 1) IN (3)
ORDER BY anon

SELECT anon, COUNT(*) AS count, 20 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING (count IN (2)) IN (true)
ORDER BY anon

SELECT COUNT(*) AS rows, 21 AS test_index
FROM (
    SELECT anon
    FROM (1, 1, 2)
    GROUP BY anon
    HAVING false
)

SELECT anon, COUNT(*) AS rows, COUNT(*) + 1 AS adjusted, 22 AS test_index
FROM (1, 1, 2)
GROUP BY anon
HAVING rows IN (2)
ORDER BY anon
