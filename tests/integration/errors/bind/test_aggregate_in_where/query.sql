input = input.1.txt

SELECT uid
FROM $input
WHERE COUNT(*) = 1
