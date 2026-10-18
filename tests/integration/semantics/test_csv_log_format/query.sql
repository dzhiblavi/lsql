SELECT anon1 AS id, anon2 AS name, Int(anon3) AS amount, anon4 AS note, 1 AS test_index
FROM input.csv
ORDER BY id

SELECT anon1 AS first_value, anon2 AS second_value, 2 AS test_index
FROM quoted.csv
ORDER BY first_value

SELECT anon1 AS id, anon4 AS note, 3 AS test_index
FROM input-crlf.csv
ORDER BY id

SELECT anon1 AS id, anon2 AS name, 4 AS test_index
FROM input.csv.gz
ORDER BY id DESC

SELECT anon1 AS id, anon2 AS name, 5 AS test_index
FROM STREAM 'cat input.csv'
ORDER BY id
