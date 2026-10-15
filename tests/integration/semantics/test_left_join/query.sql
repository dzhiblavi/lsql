left_input = input.1.txt
right_input = right.log

SELECT l.id AS id, l.value AS left_value, r.value AS right_value, 1 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
ORDER BY l.value, r.value

SELECT l.id AS id, l.value AS left_value, r.value AS right_value, 2 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON r.id = l.id
ORDER BY l.value, r.value

SELECT l.id, r.id, 3 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
ORDER BY l.value, r.value

SELECT l.value AS value, 4 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
WHERE r.id = null ORDER BY l.value

SELECT COUNT(*) AS rows, COUNT(r.id) AS matches, 5 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id

SELECT l.value AS value, COUNT(*) AS rows, COUNT(r.id) AS matches, 6 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
GROUP BY l.value ORDER BY l.value

SELECT l.id AS id, r.value AS value, 7 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
LIMIT 1

SELECT l.value AS value, r.id AS right_id, 8 AS test_index
FROM $left_input AS l LEFT JOIN (SELECT id FROM $right_input WHERE false) AS r ON l.id = r.id
ORDER BY l.value

SELECT COUNT(*) AS rows, 9 AS test_index
FROM (SELECT id FROM $left_input WHERE false) AS l
LEFT JOIN $right_input AS r ON l.id = r.id

SELECT l.id AS id, r.value AS value, 10 AS test_index
FROM $left_input AS l LEFT JOIN $left_input AS r ON l.id = r.id ORDER BY l.value

SELECT l.anon AS left_key, r.anon AS right_key, 11 AS test_index
FROM (1, 1, 2, 3) AS l LEFT JOIN (1, 1, 2) AS r ON l.anon = r.anon
ORDER BY l.anon, r.anon

SELECT l.anon AS left_key, r.anon AS right_key, 12 AS test_index
FROM ('A', 'b', 'C') AS l LEFT JOIN ('a', 'b', 'd') AS r ON LOWER(l.anon) = r.anon
ORDER BY l.anon

SELECT l.value AS left_value, r.value AS right_value, t.value AS third_value, 13 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
LEFT JOIN (SELECT id, value FROM $right_input WHERE value = 'right_a1') AS t ON r.id = t.id
ORDER BY l.value, r.value

SELECT l.id AS id, r.value AS value, 14 AS test_index
FROM $left_input AS l LEFT JOIN (
    SELECT r.id AS id, r.value AS value
    FROM $right_input AS r LEFT JOIN $left_input AS s ON r.id = s.id
) AS r ON l.id = r.id ORDER BY l.value, r.value

SELECT *, 15 AS test_index
FROM (SELECT id FROM $left_input) AS l
LEFT JOIN (SELECT id FROM $right_input) AS r ON l.id = r.id
ORDER BY l.id, r.id

SELECT l.anon AS key, r.anon AS value, 16 AS test_index
FROM ('a', 'b') AS l LEFT JOIN ('a', 'a') AS r ON l.anon = r.anon
WHERE l.anon IN ('b')

SELECT l.id AS id, r.id AS right_id, 17 AS test_index
FROM $left_input AS l LEFT JOIN (SELECT id FROM $right_input LIMIT 0) AS r ON l.id = r.id
ORDER BY l.value

SELECT COUNT(*) AS rows, 18 AS test_index
FROM $left_input AS l LEFT JOIN (SELECT id FROM $right_input WHERE false) AS r ON l.id = r.id
WHERE r.id = null

SELECT l.anon AS key, r.anon AS value, 19 AS test_index
FROM (1, 2) AS l LEFT JOIN (1, 1, 2) AS r ON l.anon = r.anon
UNION ALL
SELECT l.anon AS key, r.anon AS value, 19 AS test_index
FROM (3) AS l LEFT JOIN (4) AS r ON l.anon = r.anon

SELECT l.anon AS key, r.anon AS value, 20 AS test_index
FROM ('1', '2', 'bad') AS l LEFT JOIN ('1', 'bad') AS r ON Int(l.anon) = Int(r.anon)
ORDER BY l.anon

SELECT l.id AS id, r.id AS right_id, 21 AS test_index
FROM $left_input AS l LEFT JOIN empty.log AS r ON l.id = r.id
ORDER BY l.value

SELECT j.l.anon AS key, j.r.anon AS value, t.anon AS third, 22 AS test_index
FROM (SELECT * FROM (1, 2) AS l LEFT JOIN (1) AS r ON l.anon = r.anon) AS j
LEFT JOIN (1, 2) AS t ON j.l.anon = t.anon
ORDER BY j.l.anon

SELECT l.id AS id, r.value AS value, 23 AS test_index
FROM input.1.txt AS l LEFT JOIN right.log AS r ON l.id = r.id
WHERE r.value = 'right_a2'

SELECT COUNT(*) AS rows, 24 AS test_index
FROM empty.log AS l LEFT JOIN $right_input AS r ON l.id = r.id

SELECT left_id, right_id, 25 AS test_index
FROM (SELECT anon AS left_id FROM (1, 2, 3))
LEFT JOIN (SELECT anon AS right_id FROM (1, 1, 2)) ON left_id = right_id

SELECT l.anon AS left_key, r.anon AS right_key, 26 AS test_index
FROM ('A', 'b') AS l LEFT JOIN ('a', 'B') AS r ON LOWER(r.anon) = LOWER(l.anon)

SELECT l.anon AS left_key, r.anon AS right_key, 27 AS test_index
FROM (1, 2) AS l LEFT JOIN (3, 4) AS r ON 1 = 1

SELECT l.anon AS left_key, r.anon AS right_key, 28 AS test_index
FROM (1, 2) AS l LEFT JOIN (3, 4) AS r ON 1 = 2

SELECT l.anon AS left_key, r.anon AS right_key, 29 AS test_index
FROM (1, 2) AS l LEFT JOIN (8, 9) AS r ON l.anon = 1

SELECT l.anon AS left_key, r.anon AS right_key, 30 AS test_index
FROM (1, 2) AS l LEFT JOIN (8, 9) AS r ON l.anon = null

SELECT *, 31 AS test_index
FROM (1, 2) AS l LEFT JOIN (true) AS r ON (l.anon IN (1)) = r.anon

SELECT *, 32 AS test_index
FROM (1, 2) AS l LEFT JOIN (1, 3) AS r ON (l.anon IN (1)) = (r.anon IN (1))

SELECT l.anon AS left_key, r.n AS matches, 33 AS test_index
FROM (SELECT anon, COUNT(*) AS n FROM (1, 1, 2) GROUP BY anon) AS l
LEFT JOIN (SELECT 2 AS n FROM (1)) AS r ON l.n = r.n ORDER BY l.anon
