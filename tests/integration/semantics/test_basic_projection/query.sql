input = input.1.txt

---------------------

SELECT uid, status, 1 AS test_index
FROM $input

SELECT user AS username, action AS event, 2 AS test_index
FROM $input

SELECT 'literal' AS kind, 42 AS answer, 3 AS test_index
FROM $input
LIMIT 1

SELECT *, 4 AS test_index
FROM $input

SELECT 42 AS answer, 5 AS test_index
FROM (1, 1, 2)

SELECT uid AS first, uid AS second, 6 AS test_index
FROM $input
ORDER BY uid

SELECT status AS uid, uid AS status, 7 AS test_index
FROM $input
ORDER BY uid
