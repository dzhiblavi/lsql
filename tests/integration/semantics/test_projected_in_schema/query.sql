input = input.1.txt

ok_users = (
    SELECT uid
    FROM $input
    WHERE status = 'ok'
)

---------------------

SELECT uid, uid IN $ok_users AS is_ok
FROM $input
WHERE status != 'missing'
ORDER BY uid
