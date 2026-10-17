input = input.1.txt

projected = (
    SELECT
        uid AS user_id,
        status AS state,
        action AS emitted_action
    FROM $input
)

ok_users = (
    SELECT user_id
    FROM $projected
    WHERE state = 'ok'
)

---------------------

SELECT emitted_action AS action, 1 AS test_index
FROM $projected
WHERE user_id IN $ok_users

SELECT chosen_id, chosen_action, 2 AS test_index
FROM (
    SELECT user_id AS chosen_id, emitted_action AS chosen_action, state AS chosen_state
    FROM (
        SELECT uid AS user_id, action AS emitted_action, status AS state
        FROM input.1.txt
    )
)
WHERE LOWER(COALESCE(chosen_state, '')) = 'ok' AND chosen_id != 'ccc'
ORDER BY chosen_id

SELECT uid, status, 3 AS test_index
FROM (SELECT uid AS status, status AS uid FROM input.1.txt)
WHERE status = 'aaa' AND uid = 'ok'

SELECT first_uid, second_uid, 4 AS test_index
FROM (SELECT uid AS first_uid, uid AS second_uid FROM input.1.txt)
WHERE first_uid = 'aaa' OR second_uid = 'ccc'
ORDER BY second_uid

SELECT user_id, state, 5 AS test_index
FROM (SELECT uid AS user_id, LOWER(status) AS state FROM input.1.txt)
WHERE state = 'ok'
ORDER BY user_id

SELECT COUNT(*) AS rows, 6 AS test_index
FROM (SELECT uid AS user_id FROM input.1.txt LIMIT 2)
WHERE user_id = 'ccc'

SELECT user_id, 7 AS test_index
FROM (
    SELECT uid AS user_id FROM input.1.txt
    UNION ALL
    SELECT action AS user_id FROM input.1.txt
)
WHERE user_id = 'aaa' OR user_id = 'login'
ORDER BY user_id
