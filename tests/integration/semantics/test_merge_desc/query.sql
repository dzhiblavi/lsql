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

SELECT anon, 5 AS test_index
FROM ((4, 3, 3, 1) UNION ALL SORTED BY anon DESC (3, 2, 1))
WHERE anon > 1
LIMIT 4

SELECT anon, 6 AS test_index
FROM ((1, 2, 3) UNION ALL SORTED BY (anon) (2, 3) UNION ALL SORTED BY (anon) (1, 3))
WHERE anon >= 2

SELECT COUNT(*) AS rows, 7 AS test_index
FROM ((1, 2) UNION ALL SORTED BY (anon) (3, 4))
WHERE anon > 10
