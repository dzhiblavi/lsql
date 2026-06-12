input = input.1.txt

failed = (
    SELECT uid
    FROM $input
    WHERE status = 'fail'
)

SELECT COUNT(*) AS count
FROM $input
WHERE uid IN $failed
