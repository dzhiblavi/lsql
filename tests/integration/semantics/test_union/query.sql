input1 = input.1.txt
input2 = input.2.txt

merged = (
    $input1
    UNION ALL
    $input2
)

sorted = (
    SELECT *
    FROM $input1
    ORDER BY timestamp

    UNION ALL SORTED BY timestamp

    SELECT *
    FROM $input2
    ORDER BY timestamp
)

---------------------

SELECT uid, source, 1 AS test_index
FROM $merged

SELECT timestamp, uid, source, 2 AS test_index
FROM $sorted

SELECT *, 3 AS test_index
FROM $merged
WHERE uid IN ('aaa', 'ddd')

SELECT uid, status, path, 4 AS test_index
FROM $merged
