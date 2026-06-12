input = input.1.txt

SELECT bucket, COUNT(*) AS count, MAX(Int(latency)) AS max_latency
FROM $input
GROUP BY bucket
ORDER BY count, bucket
LIMIT 100
