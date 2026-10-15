SELECT key, COUNT(key) AS values
FROM ('1', '01', 'bad', 'bad', '2')
GROUP BY Int(anon) AS key
ORDER BY key
