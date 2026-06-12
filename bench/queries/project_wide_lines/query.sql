input = input.1.txt

SELECT uid, status, latency
FROM $input
WHERE status = 'ok'
ORDER BY uid
LIMIT 100
