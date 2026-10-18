def MiNuTe_S = 60
def add x y = x + y + 2
def shifted x y = add(x, y) + minute_s()
def twice x = x + x
def identity x = x
def first x y = x
def reverse_add x y = add(y, x)
def maximum x = MAX(x)
def separator = ':'
def json_path = '$.value'
def equal x y = x = y
def small x = x IN (1, 2)
def member x = x IN (
    SELECT identity(anon) AS key FROM (1, 2)
    WHERE small(anon)
)
def grouped_member x = x IN (
    SELECT key FROM (1, 1, 2, 3)
    WHERE small(anon)
    GROUP BY identity(anon) AS key
    HAVING equal(MAX(anon), 2)
    ORDER BY identity(key) DESC
    LIMIT 1
)
def sum5 a b c d e = a + b + c + d + e

SELECT add(3, 4) AS result, minute_s, MINUTE_S() AS seconds, 1 AS test_index
FROM (SELECT 7 AS minute_s FROM (0))

SELECT shifted(1, 2) AS result, 2 AS test_index FROM (0)

SELECT twice(anon + 1) AS result, 3 AS test_index FROM (1, 2)
ORDER BY result

SELECT first(y, x) AS result, 4 AS test_index
FROM (SELECT 1 AS x, 2 AS y FROM (0))

SELECT reverse_add(anon, 1) AS result, 5 AS test_index FROM (3)

SELECT identity(anon) AS text, identity(Int(anon)) AS number, 6 AS test_index
FROM ('3')

SELECT SUBSTR(anon, identity(1), identity(2)) AS part,
       SPLITPART(anon, separator(), identity(1)) AS suffix, 7 AS test_index
FROM ('abc:def')

SELECT JSON_VALUE(anon, json_path()) AS value, 8 AS test_index
FROM ('{"value":42}')

SELECT maximum(anon) AS result, identity(MAX(anon)) AS repeated, 9 AS test_index
FROM (1, 3, 2)

SELECT key, maximum(anon) AS result, 10 AS test_index FROM (1, 1, 2)
WHERE small(anon)
GROUP BY identity(anon) AS key
HAVING equal(MAX(anon), 2)
ORDER BY identity(key)

SELECT l.anon AS left_key, r.anon AS right_key, 11 AS test_index
FROM (1, 2) AS l LEFT JOIN (2) AS r ON equal(l.anon, r.anon)
ORDER BY l.anon

SELECT anon, 12 AS test_index FROM (1, 2, 3) WHERE small(anon)
ORDER BY anon

SELECT anon, member(anon) AS result, member(anon) AS repeated, 13 AS test_index
FROM (1, 2, 3) ORDER BY anon

SELECT anon, grouped_member(anon) AS result, 14 AS test_index
FROM (1, 2, 3) ORDER BY anon

input = (SELECT identity(anon) AS value FROM (1, 2))
SELECT identity(value) AS result, 15 AS test_index FROM $input
ORDER BY identity(value) DESC

SELECT anon AS value, 16 AS test_index FROM (1, 3)
UNION ALL SORTED BY identity(value)
SELECT anon AS value, 16 AS test_index FROM (2, 4)

SELECT sum5(1, 2, 3, 4, 5) AS result, 17 AS test_index FROM (0)

def later x = twice(x) + 1
SELECT later(2) AS result, later(3) AS another, 18 AS test_index FROM (0)

def projected_member x = 1 IN (SELECT x FROM (0))
SELECT projected_member(1) AS result, projected_member(2) AS another,
       19 AS test_index FROM (0)

def acceptable x = NOT x LIKE 'bad'
SELECT anon, acceptable(anon) AS result, 20 AS test_index FROM ('good', 'bad')
ORDER BY anon
