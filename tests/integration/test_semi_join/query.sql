SELECT anon
FROM ('a', 'b', 'c')
WHERE anon IN ('b', 'c')

SELECT anon
FROM ('a', 'b', 'c')
WHERE String(anon) IN ('b') OR String(anon) = 'a'

SELECT anon
FROM ('a', 'b', 'c')
WHERE ! anon IN ('b')

SELECT anon, anon IN ('b', 'c') AS hit
FROM ('a', 'b', 'c')

SELECT anon, COALESCE(anon IN ('b'), false) AS hit
FROM ('a', 'b')

SELECT anon, anon IN ('b') AS hit
FROM ('a', 'b')
