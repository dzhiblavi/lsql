input = input.1.txt

SELECT uid
FROM $input
WHERE uid IN (
    SELECT uid, status
    FROM $input
)
