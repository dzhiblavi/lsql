SELECT key, COUNT(key) AS values
FROM ('1', '01', 'bad', 'bad', '2')
GROUP BY Int(anon) AS key
ORDER BY key

SELECT key, COUNT(key) AS n, 2 AS test_index
FROM (1, 1, 2)
GROUP BY anon AS key
ORDER BY key

SELECT anon, COUNT(anon) AS n, SUM(anon) AS total, 3 AS test_index
FROM (1, 1, 2)
GROUP BY anon + 1 AS anon
ORDER BY anon

SELECT anon, COUNT(anon) AS n, SUM(anon) AS total, 4 AS test_index
FROM (1, 1, 2, 3)
WHERE anon = 1
GROUP BY anon + 1 AS anon

SELECT key, COUNT(key) AS n, SUM(Int(anon)) AS total, 5 AS test_index
FROM ('1', '01', '2')
GROUP BY Int(anon) AS key
ORDER BY key

SELECT key, COUNT(key) + 1 AS shifted, 6 AS test_index
FROM ('1', '01', 'bad', '2')
GROUP BY Int(anon) AS key
ORDER BY key

SELECT *, COUNT(*) AS n, 7 AS test_index
FROM (SELECT anon AS id, anon + 10 AS payload FROM (1, 1, 2))
GROUP BY id
ORDER BY id

SELECT COUNT(key) AS n, 8 AS test_index
FROM (1, 1, 2)
GROUP BY anon AS key
ORDER BY key

SELECT selected, COUNT(selected) AS n, 9 AS test_index
FROM (1, 1, 2)
GROUP BY anon IN (1) AS selected
ORDER BY selected

SELECT key, COUNT(key IN (1)) AS n, 10 AS test_index
FROM (1, 1, 2)
GROUP BY anon AS key
ORDER BY key

SELECT key, COUNT(key) AS n, 11 AS test_index
FROM ('1', '01', 'bad', 'bad', '2')
GROUP BY Int(anon) AS key
HAVING COUNT(key) > 1
ORDER BY key

SELECT anon, original, COUNT(anon) AS n, 12 AS test_index
FROM (1, 1, 2)
GROUP BY anon + 1 AS anon, anon AS original
ORDER BY original

SELECT value, original, COUNT(value) AS n, SUM(original) AS total, 13 AS test_index
FROM (SELECT anon AS id, anon + 10 AS value FROM (1, 2))
GROUP BY id AS value, value AS original
ORDER BY value

SELECT anon, COUNT(anon) AS n, 14 AS test_index
FROM (1, 2, 3)
WHERE anon IN (1, 2)
GROUP BY anon + 1 AS anon
ORDER BY anon

SELECT COUNT(*) AS rows, 15 AS test_index
FROM (
    SELECT key, COUNT(key) AS n
    FROM (1, 2)
    WHERE false
    GROUP BY anon AS key
)

SELECT key, COUNT(key) AS n, COUNT(*) AS rows, 16 AS test_index
FROM (1, 2, 3)
GROUP BY 1 AS key

SELECT COUNT(*) AS n, 17 AS test_index
FROM (1, 1, 2)
GROUP BY anon AS key
HAVING key = 1
ORDER BY key

SELECT key, COUNT(key) AS n, 18 AS test_index
FROM (1, 3, 2)
GROUP BY anon AS key
ORDER BY key IN (2), key
