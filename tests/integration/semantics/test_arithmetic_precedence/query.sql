SELECT 1 + 2 = 3 AS result
FROM (1)

SELECT
    8 - 3 + 1 AS add_subtract,
    8 - (3 + 1) AS grouped_add_subtract,
    16 / 4 / 2 AS divide,
    16 / (4 / 2) AS grouped_divide,
    1 + 8 / 2 AS divide_before_add,
    8 / 2 - 1 AS divide_before_subtract,
    2 AS test_index
FROM (1)

SELECT
    1 + 2 = 3 AS equal,
    1 + 2 != 4 AS not_equal,
    1 + 2 < 4 AS less,
    1 + 2 <= 3 AS less_equal,
    5 - 2 > 2 AS greater,
    5 - 2 >= 3 AS greater_equal,
    1 + 2 IN (3, 4) AS member,
    'a' + 'b' LIKE 'ab' AS matches,
    3 AS test_index
FROM (1)

SELECT anon, 4 AS test_index
FROM (1, 2, 3)
WHERE anon + 1 = 3
ORDER BY anon

SELECT l.anon AS left_key, r.anon AS right_key, 5 AS test_index
FROM (1, 2) AS l LEFT JOIN (2, 3) AS r ON l.anon + 1 = r.anon
ORDER BY l.anon
