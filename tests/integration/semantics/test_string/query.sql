input = input.1.txt

---------------------

SELECT
    uid,
    RSUBSTR(request, 'user=[\d]+') AS user_id,
    1 AS test_index
FROM $input

SELECT
    uid,
    RSUBSTR(request, 'session=[a-z]+') AS session_id,
    2 AS test_index
FROM $input

SELECT
    uid,
    COALESCE(RSUBSTR(request, 'user=[\d]+'), 'missing') AS user_id,
    3 AS test_index
FROM $input
