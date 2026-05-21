SELECT anon1
FROM ('a', 'b', 'c')
WHERE anon1 IN ('b', 'c')

SELECT anon1
FROM ('a', 'b', 'c')
WHERE String(anon1) IN ('b') OR String(anon1) = 'a'

SELECT anon1
FROM ('a', 'b', 'c')
WHERE ! anon1 IN ('b')

SELECT anon1, anon1 IN ('b', 'c') AS hit
FROM ('a', 'b', 'c')

SELECT anon1, COALESCE(anon1 IN ('b'), false) AS hit
FROM ('a', 'b')

SELECT anon1, anon1 IN ('b') AS hit
FROM ('a', 'b')
