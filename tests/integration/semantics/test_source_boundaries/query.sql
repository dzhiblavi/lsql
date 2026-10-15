SELECT COUNT(*) AS rows, 1 AS test_index
FROM empty.log

SELECT COUNT(*) AS rows, 2 AS test_index
FROM empty.log.gz

SELECT COUNT(*) AS rows, 3 AS test_index
FROM STREAM 'cat empty.log'

SELECT _line AS line, 4 AS test_index
FROM partial.log

SELECT _line AS line, 5 AS test_index
FROM partial.log.gz

SELECT _line AS line, 6 AS test_index
FROM STREAM 'cat partial.log'

SELECT COUNT(*) AS rows, 7 AS test_index
FROM unterminated.log

SELECT _line AS line, 8 AS test_index
FROM unterminated.log.gz

SELECT _line AS line, 9 AS test_index
FROM STREAM 'cat unterminated.log'

SELECT _line AS line, 10 AS test_index
FROM crlf.log

SELECT _line AS line, 11 AS test_index
FROM crlf.log.gz

SELECT _line AS line, 12 AS test_index
FROM STREAM 'cat crlf.log'

SELECT
    SUBSTR(_line, 0, 4) AS head,
    SUBSTR(_line, 131076, 5) AS tail,
    SUBSTR(_line, 131081) AS extra,
    13 AS test_index
FROM long.log
ORDER BY head

SELECT
    SUBSTR(_line, 0, 4) AS head,
    SUBSTR(_line, 131076, 5) AS tail,
    SUBSTR(_line, 131081) AS extra,
    14 AS test_index
FROM long.log.gz
ORDER BY head

SELECT
    SUBSTR(_line, 0, 4) AS head,
    SUBSTR(_line, 131076, 5) AS tail,
    SUBSTR(_line, 131081) AS extra,
    15 AS test_index
FROM STREAM 'cat long.log'
ORDER BY head

SELECT SUBSTR(_line, 0, 1) AS head, RSUBSTR(_line, 'end_[abc]$') AS tail, 16 AS test_index
FROM pages.log
ORDER BY head

SELECT SUBSTR(_line, 0, 1) AS head, RSUBSTR(_line, 'end_[abc]$') AS tail, 17 AS test_index
FROM pages.log.gz
ORDER BY head

SELECT SUBSTR(_line, 0, 1) AS head, RSUBSTR(_line, 'end_[abc]$') AS tail, 18 AS test_index
FROM STREAM 'cat pages.log'
ORDER BY head
