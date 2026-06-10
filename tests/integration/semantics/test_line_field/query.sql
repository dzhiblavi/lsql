input = input.1.txt

---------------------

SELECT uid, _line, 1 AS test_index
FROM $input
WHERE uid = 'aaa'
