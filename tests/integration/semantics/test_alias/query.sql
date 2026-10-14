input = requests.log
known_fields = (
    SELECT uid AS user_id, 7 AS rank FROM $input
)
left_alias = $input AS l
right_alias = responses.log AS r
nested_alias = $left_alias AS outer_name
value_alias = (1, 2, 3) AS v
late_alias = (
    SELECT * FROM responses.log AS late_src
)
forwarded_alias = $late_alias AS p

SELECT l.uid AS uid, l.status AS status, 1 AS test_index
FROM $input AS l ORDER BY l.uid

SELECT k.rank AS rank, k.user_id AS uid, 2 AS test_index
FROM $known_fields AS k ORDER BY k.rank, k.user_id

SELECT l.uid, l.status, 3 AS test_index
FROM $left_alias ORDER BY l.uid

SELECT outer_name.l.uid AS uid, 4 AS test_index
FROM $nested_alias ORDER BY outer_name.l.uid

SELECT r.uid AS uid, 5 AS test_index
FROM $right_alias WHERE r.uid = 'xxx'

SELECT *, 6 AS test_index
FROM $right_alias ORDER BY r.uid

SELECT l.uid AS uid, l.uid IN ('aaa', 'ccc') AS is_selected, 7 AS test_index
FROM $input AS l ORDER BY l.uid

SELECT a.anon, 8 AS test_index
FROM (1, 2, 3) AS a WHERE a.anon IN (1, 3) ORDER BY a.anon

SELECT l.status AS status, COUNT(*) AS count, 9 AS test_index
FROM $left_alias GROUP BY l.status HAVING count >= 1 ORDER BY l.status

SELECT m.uid AS uid, 10 AS test_index
FROM MATERIALIZE(SELECT uid FROM $input) AS m ORDER BY m.uid

SELECT s.uid, 11 AS test_index
FROM STREAM 'printf "[2026-May-06 12:00:01.000000] uid=stream_one\n[2026-May-06 12:00:02.000000] uid=stream_two\n"' AS s
ORDER BY s.uid

SELECT f.uid AS uid, 12 AS test_index
FROM requests.log AS f ORDER BY f.uid

SELECT f.uid AS uid, 13 AS test_index
FROM requests.log @ 2026-05-06T12:00:01 + 1 AS f ORDER BY f.uid

SELECT v.anon, 14 AS test_index
FROM $value_alias ORDER BY v.anon

SELECT p.late_src.uid AS uid, 15 AS test_index
FROM $forwarded_alias ORDER BY p.late_src.uid

SELECT uid, 16 AS test_index
FROM (SELECT l.uid AS uid FROM $left_alias) ORDER BY uid

SELECT l.uid AS uid, 17 AS test_index
FROM $left_alias
WHERE l.uid IN (SELECT k.user_id FROM $known_fields AS k WHERE k.rank = 7)
ORDER BY l.uid

SELECT u.uid AS uid, 18 AS test_index
FROM (
    SELECT a.uid AS uid FROM $input AS a
    UNION ALL
    SELECT b.uid AS uid FROM responses.log AS b
) AS u ORDER BY u.uid

('literal') AS top
