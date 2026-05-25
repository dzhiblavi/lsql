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
