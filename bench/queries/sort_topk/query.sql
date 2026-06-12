input = input.1.txt

SELECT uid, latency
FROM $input
ORDER BY latency, uid
LIMIT 100
