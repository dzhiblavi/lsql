SELECT uid, 1 AS test_index
FROM input.1.txt
ORDER BY uid

SELECT uid, 2 AS test_index
FROM input.jsonl 'ISO8601'
ORDER BY uid

SELECT uid, 3 AS test_index
FROM input.jsonl @ 2026-05-06T12:00:02 'ISO8601'
ORDER BY uid

SELECT uid, 4 AS test_index
FROM input.jsonl @ 2026-05-06T12:00:01 + 3 'ISO8601'
ORDER BY uid

SELECT uid, 5 AS test_index
FROM input.1.txt @ 2026-05-06T12:00:02
ORDER BY uid
