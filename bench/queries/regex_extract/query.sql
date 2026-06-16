input = input.1.txt

SELECT
    COUNT(*) AS matched,
    COUNT(RSUBSTR(request, 'user=[\d]+')) AS user_ids,
    COUNT(RSUBSTR(request, 'session=s[\d]+')) AS session_ids
FROM $input
WHERE uid LIKE 'user[0-9]+'
