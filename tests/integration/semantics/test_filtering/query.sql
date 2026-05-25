input = input.1.txt

---------------------

SELECT uid, status, 1 AS test_index
FROM $input
WHERE status = 'ok'

SELECT uid, status, 2 AS test_index
FROM $input
WHERE status = 'ok' AND uid = 'aaa'

SELECT uid, status, 3 AS test_index
FROM $input
WHERE status = 'ok' OR uid = 'bbb'

SELECT uid, status, 4 AS test_index
FROM $input
WHERE NOT status = 'ok'

SELECT uid, 5 AS test_index
FROM $input
WHERE uid IN ('aaa', 'ccc')

SELECT uid, path, 6 AS test_index
FROM $input
WHERE path LIKE '/api/.*'

SELECT uid, missing, 7 AS test_index
FROM $input
WHERE missing = null
