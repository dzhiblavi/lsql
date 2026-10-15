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
