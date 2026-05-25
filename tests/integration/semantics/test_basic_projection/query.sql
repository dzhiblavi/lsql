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
