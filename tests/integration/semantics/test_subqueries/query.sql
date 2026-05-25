input = input.1.txt

filtered = (
    SELECT uid, status
    FROM $input
    WHERE status = 'ok'
)

renamed = (
    SELECT uid AS user_id
    FROM $filtered
)

---------------------

SELECT uid, status, 1 AS test_index
FROM $filtered

SELECT user_id, 2 AS test_index
FROM $renamed

SELECT uid, 3 AS test_index
FROM (
    SELECT uid
    FROM $input
    WHERE kind = 'api'
)

SELECT uid, status, 4 AS test_index
FROM (
    SELECT uid, status
    FROM $input
)
WHERE uid IN ('aaa', 'ccc')
