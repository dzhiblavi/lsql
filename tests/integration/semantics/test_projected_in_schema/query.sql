input = input.1.txt
late_users = input.1.txt
late_forwarded = (
    SELECT * FROM $late_users
)

ok_users = (
    SELECT uid
    FROM $input
    WHERE status = 'ok'
)

---------------------

SELECT uid, uid IN $ok_users AS is_ok
FROM $input
WHERE status != 'missing'
ORDER BY uid

SELECT uid, uid IN $late_forwarded AS is_known, 1 AS test_index
FROM $input
ORDER BY uid

SELECT uid, 2 AS test_index
FROM $late_forwarded
ORDER BY uid
