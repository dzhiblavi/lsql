input = input.1.txt@2026-05-06T12:00:02+2

---------------------

SELECT timestamp, uid, status, 1 AS test_index
FROM $input
ORDER BY timestamp

SELECT COUNT(*) AS count, 2 AS test_index
FROM $input

SELECT uid, 3 AS test_index
FROM input.1.txt@2026-05-06T12:00:02+1
ORDER BY timestamp

SELECT COUNT(*) AS rows, 4 AS test_index
FROM input.1.txt@2026-05-06T12:01:00+1

SELECT uid, 5 AS test_index
FROM input.1.txt@2026-05-06T11:59:00+120
ORDER BY timestamp
