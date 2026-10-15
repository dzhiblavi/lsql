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

SELECT anon, anon IN (SELECT anon FROM (1) WHERE false) AS member, 11 AS test_index
FROM (1, 2)

SELECT COUNT(*) AS rows, 12 AS test_index
FROM (SELECT anon FROM (1, 2) WHERE anon IN (SELECT anon FROM (1) WHERE false))

SELECT anon, anon IN (1, 1) AS member, 13 AS test_index
FROM (1, 1, 2)
ORDER BY anon

SELECT anon, 14 AS test_index
FROM (1, 1, 2)
WHERE anon IN (1, 1)
ORDER BY anon

SELECT
    anon AS text,
    Int(anon) IN (SELECT Int(anon) AS key FROM ('bad', '1')) AS member,
    15 AS test_index
FROM ('bad', '1', '2')

SELECT anon AS text, 16 AS test_index
FROM ('bad', '1', '2')
WHERE Int(anon) IN (SELECT Int(anon) AS key FROM ('bad', '1'))
ORDER BY anon
