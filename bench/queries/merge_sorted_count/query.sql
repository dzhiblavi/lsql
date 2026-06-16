left = input.1.txt
right = input.2.txt

SELECT COUNT(*) AS count
FROM ($left UNION ALL SORTED BY timestamp $right)
