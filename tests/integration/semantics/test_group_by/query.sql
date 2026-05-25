input = input.1.txt

---------------------

SELECT status, COUNT(true) AS count, 1 AS test_index
FROM $input
GROUP BY status
ORDER BY status

SELECT status, kind, COUNT(true) AS count, 2 AS test_index
FROM $input
GROUP BY status, kind
ORDER BY count, kind DESC

SELECT status AS state, COUNT(true) AS count, 3 AS test_index
FROM $input
GROUP BY status
ORDER BY count DESC
