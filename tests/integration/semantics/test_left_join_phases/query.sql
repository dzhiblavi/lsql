left_input = MATERIALIZE(STREAM 'printf "[2026-May-06 12:00:01.000000] id=long_key_a_1234567890 value=left_a_12345678901234567890\n[2026-May-06 12:00:02.000000] id=long_key_b_1234567890 value=left_b_12345678901234567890\n"')
right_input = MATERIALIZE(STREAM 'printf "[2026-May-06 12:00:01.000000] id=long_key_a_1234567890 value=right_a1_12345678901234567890\n[2026-May-06 12:00:02.000000] id=long_key_a_1234567890 value=right_a2_12345678901234567890\n"')

joined = MATERIALIZE(
    SELECT l.id AS id, l.value AS left_value, r.value AS right_value
    FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
)

SELECT id, left_value, 1 AS test_index FROM $joined ORDER BY id

SELECT id, right_value, 2 AS test_index FROM $joined ORDER BY id, right_value

SELECT l.value AS left_value, j.right_value AS right_value, 3 AS test_index
FROM $left_input AS l LEFT JOIN $joined AS j ON l.id = j.id
ORDER BY l.value, j.right_value

SELECT j.right_value AS right_value, r.value AS value, 4 AS test_index
FROM $joined AS j LEFT JOIN $right_input AS r ON j.id = r.id
ORDER BY j.left_value, j.right_value, r.value

SELECT l.id AS id, r.value AS value, 5 AS test_index
FROM $left_input AS l LEFT JOIN $right_input AS r ON l.id = r.id
WHERE l.id IN (SELECT id FROM $joined WHERE right_value != null)
ORDER BY l.id, r.value

SELECT COUNT(*) AS rows, COUNT(j.right_value) AS matches, 6 AS test_index
FROM $joined AS j LEFT JOIN $left_input AS l ON j.id = l.id

SELECT l.id AS id, r.value AS value, 7 AS test_index
FROM STREAM 'printf "[2026-May-06 12:00:01.000000] id=a\n[2026-May-06 12:00:02.000000] id=b\n"' AS l
LEFT JOIN STREAM 'printf "[2026-May-06 12:00:01.000000] id=a value=one\n"' AS r ON l.id = r.id
ORDER BY l.id
