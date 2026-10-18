SELECT id, user, count, ratio, enabled, nothing, meta, events, dup, huge, 1 AS test_index
FROM input.jsonl
ORDER BY id

SELECT id, Int(count) AS count_i, Float(ratio) AS ratio_f, Int(huge) AS huge_i,
       JSON_VALUE(meta, 'code') AS code,
       JSON_VALUE(events, '[0].message') AS message, 2 AS test_index
FROM input.jsonl
ORDER BY id

SELECT id, _line, 3 AS test_index
FROM input.jsonl
WHERE id = '001'

SELECT id, user, meta, 4 AS test_index
FROM input-crlf.jsonl
ORDER BY id

SELECT id, user, meta, 5 AS test_index
FROM input.jsonl.gz
ORDER BY id DESC

SELECT id, user, meta, 6 AS test_index
FROM STREAM 'cat input.jsonl'
ORDER BY id

SELECT id, count, enabled, 7 AS test_index
FROM empty.jsonl
ORDER BY id
