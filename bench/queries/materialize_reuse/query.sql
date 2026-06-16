input = input.1.txt

failed = MATERIALIZE (
    SELECT uid
    FROM $input
    WHERE status = 'fail'
)

SELECT COUNT(*) AS count
FROM $failed

SELECT COUNT(*) AS count
FROM $input
WHERE uid IN $failed
