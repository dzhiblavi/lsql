input = input.1.txt

---------------------

SELECT
    uid,
    RSUBSTR(request, 'user=[\d]+') AS user_id,
    1 AS test_index
FROM $input

SELECT
    uid,
    RSUBSTR(request, 'session=[a-z]+') AS session_id,
    2 AS test_index
FROM $input

SELECT
    uid,
    COALESCE(RSUBSTR(request, 'user=[\d]+'), 'missing') AS user_id,
    3 AS test_index
FROM $input

SELECT
    anon AS text,
    SUBSTR(anon, 0, 2) AS prefix,
    SUBSTR(anon, 2) AS suffix,
    SUBSTR(anon, 6) AS at_end,
    SUBSTR(anon, 1, 0) AS empty,
    SUBSTR(anon, 4, 99) AS clipped,
    4 AS test_index
FROM ('abcdef', 'uvwxyz')

SELECT
    SPLITPART(anon, ',', 0) AS first,
    SPLITPART(anon, ',', 1) AS second,
    SPLITPART(anon, ',', 2) AS third,
    SPLITPART(anon, ',', 4) AS last,
    SPLITPART(anon, ',', 5) AS missing,
    5 AS test_index
FROM (',a,,b,')

SELECT anon AS text, LOWER(anon) AS lowered, RSUBSTR(anon, '[0-9]+') AS digits, 6 AS test_index
FROM ('', 'ABC', 'ab12cd34')
