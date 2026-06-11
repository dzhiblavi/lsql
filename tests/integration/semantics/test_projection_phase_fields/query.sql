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
