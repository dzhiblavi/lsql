input = input.1.txt

SELECT status_code, COUNT(*) AS count, MAX(Int(total_time)) AS max_time
FROM $input
WHERE status_code != '200'
GROUP BY status_code
ORDER BY count DESC
