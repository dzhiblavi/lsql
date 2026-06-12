input = input.1.txt

SELECT
    uid,
    RSUBSTR(message, 'user=[\d]+') AS user_id,
    RSUBSTR(message, 'session=s[\d]+') AS session_id
FROM $input
WHERE message LIKE '.*session=s[0-9]+.*'
ORDER BY uid
LIMIT 100
