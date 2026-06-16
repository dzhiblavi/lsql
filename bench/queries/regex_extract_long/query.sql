input = input.1.txt

SELECT
    COUNT(*) AS matched,
    COUNT(RSUBSTR(message, 'user=[\d]+')) AS user_ids,
    COUNT(RSUBSTR(message, 'session=s[\d]+')) AS session_ids
FROM $input
WHERE message LIKE '.*session=s[0-9]+.*'
