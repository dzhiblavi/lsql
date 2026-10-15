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

SELECT anon, 2 AS test_index
FROM (SELECT anon FROM (1, 3, 3) ORDER BY anon DESC UNION ALL SORTED BY anon DESC SELECT anon FROM (2, 3) ORDER BY anon DESC)

SELECT anon, 3 AS test_index
FROM (SELECT anon FROM (9) WHERE false ORDER BY anon DESC UNION ALL SORTED BY anon DESC SELECT anon FROM (1, 2) ORDER BY anon DESC)

SELECT anon, 4 AS test_index
FROM (SELECT anon FROM (1, 2) ORDER BY anon DESC UNION ALL SORTED BY anon DESC SELECT anon FROM (9) WHERE false ORDER BY anon DESC)
