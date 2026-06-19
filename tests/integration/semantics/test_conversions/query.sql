input = input.1.txt

---------------------

SELECT uid, Int(code) AS code_i, 1 AS test_index
FROM $input

SELECT uid, Float(latency) AS latency_f, 2 AS test_index
FROM $input

SELECT uid, String(code) AS code_s, 3 AS test_index
FROM $input

SELECT uid, Int(code) AS code_i, Float(latency) AS latency_f, String(flag) AS flag_s, 4 AS test_index
FROM $input
WHERE Int(code) = 200

SELECT uid, Float(missing) AS missing_f, 5 AS test_index
FROM $input

SELECT uid, Int(missing) AS missing_i, Float(missing) AS missing_f, String(missing) AS missing_s, 6 AS test_index
FROM $input

SELECT uid, parse_timestamp(timestamp, 'ORACLE') - parse_timestamp('2026-May-06 12:00:00', 'ORACLE') AS ts_offset, 7 AS test_index
FROM $input
