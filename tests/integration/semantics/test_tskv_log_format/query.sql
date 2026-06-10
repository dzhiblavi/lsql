input = input.1.txt

---------------------

SELECT timestamp, uid, status, Int(latency) AS latency_i, 1 AS test_index
FROM $input
WHERE status = 'ok'
ORDER BY timestamp

SELECT status, COUNT(*) AS count, 2 AS test_index
FROM $input
GROUP BY status
ORDER BY status
