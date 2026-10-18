SELECT anon1 AS id, anon2 AS name, anon3 AS amount, anon4 AS note, 1 AS test_index
FROM input.tsv
ORDER BY id

SELECT anon1 AS id, anon4 AS note, 2 AS test_index
FROM input-crlf.tsv
ORDER BY id

SELECT anon1 AS id, anon2 AS name, 3 AS test_index
FROM input.tsv.gz
ORDER BY id DESC

SELECT anon1 AS id, anon2 AS name, 4 AS test_index
FROM STREAM 'cat input.tsv'
ORDER BY id
