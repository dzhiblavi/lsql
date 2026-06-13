input = input.1.txt

SELECT _line
FROM $input
WHERE status = 'ok'
ORDER BY uid
LIMIT 100
