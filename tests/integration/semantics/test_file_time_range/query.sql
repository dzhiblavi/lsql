input = input.1.txt@2026-05-06T12:00:02+2

---------------------

SELECT timestamp, uid, status, 1 AS test_index
FROM $input
ORDER BY timestamp

SELECT COUNT(*) AS count, 2 AS test_index
FROM $input

SELECT uid, 3 AS test_index
FROM input.1.txt@2026-05-06T12:00:02+1
ORDER BY timestamp

SELECT COUNT(*) AS rows, 4 AS test_index
FROM input.1.txt@2026-05-06T12:01:00+1

SELECT uid, 5 AS test_index
FROM input.1.txt@2026-05-06T11:59:00+120
ORDER BY timestamp

SELECT uid, 6 AS test_index
FROM input.jsonl @ 2026-05-06T12:00:02 + 1 'ISO8601'
ORDER BY uid

SELECT anon2 AS uid, 7 AS test_index
FROM input.csv @ 2026-05-06T12:00:02 + 1 'SQL'
ORDER BY uid

SELECT anon1 AS uid, 8 AS test_index
FROM input.tsv @ 2026-05-06T12:00:02 + 1 'ORACLE'
ORDER BY uid

SELECT uid, 9 AS test_index
FROM override.tskv @ 2026-05-06T12:00:02 + 1 'ORACLE'
ORDER BY uid

SELECT uid, 10 AS test_index
FROM override.log @ 2026-05-06T12:00:02 + 1 'ISO8601'
ORDER BY uid

SELECT _line, 11 AS test_index
FROM raw.txt @ 2026-05-06T12:00:02 + 1 'ISO8601'
ORDER BY _line

SELECT COUNT(*) AS rows, 12 AS test_index
FROM first.jsonl @ 2026-05-06T12:00:02 + 1 'ISO8601'

without_format = input.1.txt @ 2026-05-06T12:00:02 + 1
SQL = input.1.txt
with_format = input.jsonl @ 2026-05-06T12:00:02 + 1 'ISO8601'
following = input.1.txt

SELECT uid, 13 AS test_index
FROM $without_format
ORDER BY uid

SELECT COUNT(*) AS rows, 14 AS test_index
FROM $SQL

SELECT uid, 15 AS test_index
FROM $with_format
ORDER BY uid

SELECT COUNT(*) AS rows, 16 AS test_index
FROM $following

SELECT uid, 17 AS test_index
FROM input.1.txt @ 2026-05-06T12:00:02 + 1 'ORACLE'
ORDER BY uid
