input = input.1.jsonl

---------------------

SELECT
    JSON_VALUE(_line, 'messages[0].title') AS title,
    JSON_VALUE(_line, '$.count') AS count,
    Int(JSON_VALUE(_line, '$.count')) AS count_i,
    JSON_VALUE(_line, 'enabled') AS enabled,
    JSON_VALUE(_line, 'nothing') AS nothing,
    JSON_VALUE(_line, 'messages') AS container,
    JSON_VALUE(_line, 'meta') AS object,
    JSON_VALUE(_line, 'missing') AS missing,
    JSON_VALUE(_line, 'meta.a/b') AS slash,
    JSON_VALUE(_line, '/meta/til~0de') AS tilde
FROM $input
