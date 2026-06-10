input = input.1.txt@2026-05-06T12:00:02+2

---------------------

SELECT timestamp, uid, status, 1 AS test_index
FROM $input
ORDER BY timestamp

SELECT COUNT(*) AS count, 2 AS test_index
FROM $input
