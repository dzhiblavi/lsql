input = input.1.txt.gz

SELECT status_code, COUNT(*) AS count, MAX(Int(total_time)) AS max_time
FROM $input
GROUP BY status_code
ORDER BY count, status_code
