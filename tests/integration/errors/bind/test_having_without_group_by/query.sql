input = input.1.txt

SELECT COUNT(*) AS count
FROM $input
HAVING COUNT(*) > 1
