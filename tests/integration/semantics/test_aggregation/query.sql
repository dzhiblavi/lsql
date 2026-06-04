input = input.1.txt

---------------------

SELECT COUNT(*) AS count, 1 AS test_index
FROM $input

SELECT COUNT(true) AS count_true, COUNT(false) AS count_false, COUNT(null) AS count_null, 2 AS test_index
FROM $input

SELECT MIN(timestamp) AS min_ts, MAX(timestamp) AS max_ts, 3 AS test_index
FROM $input

SELECT MIN(uid) AS min_uid, MAX(uid) AS max_uid, 4 AS test_index
FROM $input

SELECT COUNT(*) AS count, MIN(timestamp) AS min_ts, MAX(timestamp) AS max_ts, 5 AS test_index
FROM $input
WHERE status = 'ok'
