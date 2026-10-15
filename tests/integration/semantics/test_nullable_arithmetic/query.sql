SELECT anon AS text, Int(anon) + 1 AS incremented, 10 - Int(anon) AS difference
FROM ('2', 'bad')

SELECT
    anon AS text,
    Int(anon) + 2 AS add_left,
    2 + Int(anon) AS add_right,
    Int(anon) - 2 AS subtract_left,
    2 - Int(anon) AS subtract_right,
    Int(anon) / 2 AS dividend,
    8 / Int(anon) AS divisor,
    2 AS test_index
FROM ('8', '0', 'bad')

SELECT
    anon AS text,
    Float(anon) + 0.5 AS add_left,
    0.5 + Float(anon) AS add_right,
    Float(anon) - 0.5 AS subtract_left,
    0.5 - Float(anon) AS subtract_right,
    Float(anon) / 0.5 AS dividend,
    5.0 / Float(anon) AS divisor,
    3 AS test_index
FROM ('2.5', '0', 'bad')

SELECT
    anon AS text,
    String(Int(anon)) + '/end' AS add_left,
    'value=' + String(Int(anon)) AS add_right,
    4 AS test_index
FROM ('1', 'bad')

SELECT
    Int('bad') + 1 AS add_left,
    1 + Int('bad') AS add_right,
    Int('bad') - 1 AS subtract_left,
    1 - Int('bad') AS subtract_right,
    Int('bad') / 2 AS dividend,
    2 / Int('bad') AS divisor,
    Float('bad') + 0.5 AS float_add,
    0.5 - Float('bad') AS float_subtract,
    Float('bad') / 0.5 AS float_divide,
    String(Int('bad')) + '/end' AS string_add,
    5 AS test_index
FROM (1)

SELECT anon AS text, 6 AS test_index
FROM ('1', 'bad')
WHERE Int(anon) + 1 = 2

SELECT
    l.anon AS left_key,
    Int(r.anon) + 1 AS incremented,
    10 - Int(r.anon) AS difference,
    10 / Int(r.anon) AS ratio,
    7 AS test_index
FROM ('1', '2') AS l LEFT JOIN ('1') AS r ON l.anon = r.anon
ORDER BY l.anon
