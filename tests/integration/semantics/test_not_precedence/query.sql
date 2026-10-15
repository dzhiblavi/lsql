SELECT NOT true AND false AS result
FROM (1)

SELECT
    NOT true OR true AS disjunction,
    NOT false AND false AS conjunction,
    NOT NOT true AS double_negation,
    NOT 1 = 2 AS comparison,
    NOT 1 + 1 = 2 AS arithmetic,
    NOT 1 IN (1, 2) AS member,
    NOT 'abc' LIKE 'a.*' AS matches,
    (NOT true) AND false AS grouped,
    !true AND false AS exclamation,
    !(1 = 2) AS grouped_exclamation,
    2 AS test_index
FROM (1)

SELECT anon, 3 AS test_index
FROM (1, 2, 3)
WHERE NOT anon IN (1, 2) AND anon = 3
ORDER BY anon

SELECT anon, 4 AS test_index
FROM ('abc', 'b', 'c')
WHERE NOT anon LIKE 'a.*' AND anon != 'c'
ORDER BY anon
