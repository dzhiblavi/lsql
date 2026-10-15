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

SELECT uid, lower(timestamp) AS timestamp_lower, splitpart(timestamp, '-', 1) AS month, 8 AS test_index
FROM $input

SELECT anon AS text, Int(anon) AS value, 9 AS test_index
FROM ('0', '-1', '+1', '', ' 1', '1 ', '12x', '9223372036854775807', '-9223372036854775808', '9223372036854775808', '-9223372036854775809')

SELECT anon AS text, Float(anon) AS value, 10 AS test_index
FROM ('1.25', '-2.5', '1e2', 'nan', 'inf', '1e1000', 'bad', '2.5ms')

SELECT anon AS text, Bool(anon) AS value, 11 AS test_index
FROM ('', '0', 'false', 'yes')

SELECT anon AS flag, Int(anon) AS number, String(anon) AS text, 12 AS test_index
FROM (true, false)
