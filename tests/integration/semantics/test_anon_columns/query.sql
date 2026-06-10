input = input.1.txt

---------------------

SELECT anon1 AS module, anon2 AS context, status, 1 AS test_index
FROM $input
ORDER BY context

SELECT anon1 AS module, COUNT(*) AS count, 2 AS test_index
FROM $input
GROUP BY anon1
ORDER BY module
