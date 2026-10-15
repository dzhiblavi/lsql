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

late_users = input.1.txt
late_forwarded = (
    SELECT * FROM $late_users
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

SELECT uid, 7 AS test_index
FROM $input
WHERE uid IN $late_forwarded

SELECT uid, 8 AS test_index
FROM $late_forwarded

SELECT uid, 9 AS test_index
FROM $input
WHERE uid IN (
    SELECT uid FROM $input WHERE uid IN ('aaa', 'ccc')
)

SELECT uid, (uid IN ('aaa', 'ccc')) IN (true) AS is_selected, 10 AS test_index
FROM $input
