input = input.1.txt

SELECT
    uid,
    RSUBSTR(request, 'user=[\d]+') AS user_id,
    RSUBSTR(request, 'session=s[\d]+') AS session_id
FROM $input
WHERE uid LIKE 'user[0-9]+'
