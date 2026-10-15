SELECT anon, COUNT(*) AS count
FROM (1, 1, 2)
GROUP BY anon
HAVING count IN (2)
ORDER BY anon
