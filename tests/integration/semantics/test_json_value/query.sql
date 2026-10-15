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

SELECT
    anon AS text,
    JSON_VALUE(anon, '/0') AS first,
    JSON_VALUE(anon, '/1') AS second,
    JSON_VALUE(anon, '/3') AS missing,
    2 AS test_index
FROM ('[1,"x",null]', '[]', '42')

SELECT
    anon AS text,
    JSON_VALUE(anon, 'value') AS value,
    JSON_VALUE(anon, 'value.child') AS child,
    3 AS test_index
FROM ('{"value":""}', '{"value":null}', '{}', '{"value":false}', '{"value":{"child":"Привет"}}')
