input = input.1.txt

---------------------

SELECT timestamp, uid, 1 AS test_index
FROM $input
ORDER BY timestamp

SELECT timestamp, uid, 2 AS test_index
FROM $input
ORDER BY timestamp DESC

SELECT timestamp, uid, 3 AS test_index
FROM $input
ORDER BY timestamp
LIMIT 2

SELECT timestamp, uid, 4 AS test_index
FROM $input
ORDER BY timestamp DESC
LIMIT 2

SELECT text, key, 5 AS test_index
FROM (SELECT anon AS text, Int(anon) AS key FROM ('2', 'bad', '1', '02'))
ORDER BY key, text

SELECT text, key, 6 AS test_index
FROM (SELECT anon AS text, Int(anon) AS key FROM ('2', 'bad', '1', '02'))
ORDER BY key, text DESC

SELECT text, key, 7 AS test_index
FROM (SELECT anon AS text, Int(anon) AS key FROM ('2', 'bad', '1', '02'))
ORDER BY key, text
LIMIT 3

SELECT COUNT(*) AS rows, 8 AS test_index
FROM (SELECT anon FROM (3, 1, 2) ORDER BY anon LIMIT 0)

SELECT anon, 9 AS test_index
FROM (3, 1, 2)
ORDER BY anon
LIMIT 1

SELECT anon, 10 AS test_index
FROM (3, 1, 2)
ORDER BY anon
LIMIT 99

SELECT anon, 11 AS test_index
FROM (SELECT anon FROM (3, 1, 2, 2, 5, 4) ORDER BY anon)
WHERE anon >= 2

SELECT anon, 12 AS test_index
FROM (SELECT anon FROM (3, 1, 2, 2, 5, 4) ORDER BY anon DESC)
WHERE anon >= 2

SELECT text, key, 13 AS test_index
FROM (
    SELECT anon AS text, Int(anon) AS key
    FROM ('2', 'bad', '1', '02', '3')
    ORDER BY key, text
)
WHERE text != '1'

SELECT COUNT(*) AS rows, 14 AS test_index
FROM (SELECT anon FROM (1, 2, 3, 4) ORDER BY anon LIMIT 2)
WHERE anon > 2

SELECT anon, 15 AS test_index
FROM (SELECT anon FROM (4, 2, 1, 3, 2) ORDER BY anon)
WHERE anon >= 2
LIMIT 3

SELECT COUNT(*) AS rows, 16 AS test_index
FROM (SELECT anon FROM (1, 2, 3, 4) ORDER BY anon DESC LIMIT 2)
WHERE anon < 3

SELECT COUNT(*) AS rows, 17 AS test_index
FROM (SELECT anon FROM (1, 2, 3) ORDER BY anon)
WHERE anon > 10

SELECT number, matched, 18 AS test_index
FROM (
    SELECT l.anon AS number, r.anon AS matched
    FROM (1, 2, 3) AS l LEFT JOIN (1, 1, 3) AS r ON l.anon = r.anon
    ORDER BY l.anon DESC
)
WHERE number >= 2
