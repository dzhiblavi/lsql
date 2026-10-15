SELECT anon AS text, Int(anon) + 1 AS incremented, 10 - Int(anon) AS difference
FROM ('2', 'bad')
