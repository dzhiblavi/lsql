input = input.1.txt

failed = (
    SELECT context_id
    FROM $input
    WHERE status = 'fail'
)

SELECT COUNT(*) AS count
FROM $input
WHERE context_id IN $failed
