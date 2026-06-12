input = input.1.txt

SELECT uid, status, action, latency
FROM $input
WHERE status = 'ok'
ORDER BY uid, action
LIMIT 100
