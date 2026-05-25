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

---------------------

SELECT *, 1 AS test_index
FROM $only_uid

SELECT *, 2 AS test_index
FROM $star_from_source
WHERE uid IN ('aaa', 'ccc')

SELECT *, 3 AS test_index
FROM $right_only
