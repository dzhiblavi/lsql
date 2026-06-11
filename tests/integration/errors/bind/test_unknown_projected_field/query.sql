input = input.1.txt

projected = (
    SELECT uid
    FROM $input
)

---------------------

SELECT status
FROM $projected
