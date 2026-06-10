input1 = input.1.txt
input2 = input.2.txt

sorted = (
    SELECT *
    FROM $input1
    ORDER BY timestamp DESC

    UNION ALL SORTED BY timestamp DESC

    SELECT *
    FROM $input2
    ORDER BY timestamp DESC
)

---------------------

SELECT timestamp, uid, source, 1 AS test_index
FROM $sorted
