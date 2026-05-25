input = input.1.txt

uids = (
    SELECT uid
    FROM $input
    WHERE status = 'ok'
)

sessions = (
    SELECT session
    FROM $input
    WHERE kind = 'api'
)

renamed_only = (
    SELECT uid AS user_id
    FROM $input
)

---------------------

SELECT uid, 1 AS test_index
FROM $input
WHERE uid IN $uids

SELECT uid, session, 2 AS test_index
FROM $input
WHERE session IN $sessions

SELECT uid, 3 AS test_index
FROM $input
WHERE uid IN ('aaa', 'ccc')

SELECT uid, 4 AS test_index
FROM $input
WHERE NOT uid IN $uids

SELECT uid, 5 AS test_index
FROM $input
WHERE uid IN (
    SELECT uid
    FROM $input
    WHERE kind = 'worker'
)

SELECT *, 6 AS test_index
FROM $renamed_only
