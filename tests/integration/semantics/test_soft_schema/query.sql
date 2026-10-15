input1 = input.1.txt
input2 = input.2.txt

only_uid = (
    SELECT uid
    FROM $input1
    WHERE status = 'ok'
)

star_from_source = (
    SELECT *
    FROM $input1
    ORDER BY timestamp
)

right_only = (
    SELECT *
    FROM $input2
    ORDER BY timestamp
)

known_fields = (
    SELECT uid AS user_id, 7 AS rank
    FROM $input1
)

forwarded_known = (
    SELECT * FROM $known_fields
)

augmented = (
    SELECT *, 'fixed' AS marker FROM $input1
)

late_input = input.2.txt
late_pass = (
    SELECT *, 'fixed' AS marker FROM $late_input
)

union_left = input.1.txt
union_right = input.2.txt

---------------------

SELECT *, 1 AS test_index
FROM $only_uid

SELECT *, 2 AS test_index
FROM $star_from_source
WHERE uid IN ('aaa', 'ccc')

SELECT *, 3 AS test_index
FROM $right_only


SELECT user_id, 4 AS test_index
FROM $known_fields
ORDER BY rank, user_id

SELECT String(4 - anon) AS anon, 5 AS test_index
FROM (1, 2, 3)
ORDER BY anon

SELECT user_id, 6 AS test_index
FROM $forwarded_known
WHERE rank = 7
ORDER BY user_id

SELECT marker, uid, 7 AS test_index
FROM $augmented
ORDER BY uid

SELECT *, 8 AS test_index
FROM $late_pass
WHERE uid = 'xxx'

SELECT uid, 9 AS test_index
FROM $union_left
ORDER BY uid

SELECT uid, 10 AS test_index
FROM ($union_left UNION ALL $union_right)
ORDER BY uid

SELECT anon, 11 AS test_index
FROM (SELECT anon + 1 AS anon FROM (3, 1, 2))
WHERE anon > 2
ORDER BY anon
