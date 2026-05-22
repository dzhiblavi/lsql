access = ../../../../../../_logs/access.log@2026-05-06T14:10:00+300
access2 = ../../../../../../_logs/access.log@2026-05-06T14:10:00+300

app = ../../../../../../_logs/app.log@2026-05-07T06:00:00+3000
app2 = ../../../../../../_logs/app.log@2026-05-07T06:00:00+3000

typed = ../../../../../../_logs/typed.log
typed2 = ../../../../../../_logs/typed.log

all = (
    $access
    UNION ALL
    $typed
)

ordered = (
    SELECT *
    FROM $access
    ORDER BY timestamp

    UNION ALL SORTED BY timestamp

    SELECT *
    FROM $typed
    ORDER BY timestamp

    UNION ALL SORTED BY timestamp

    SELECT *
    FROM $typed2
    ORDER BY timestamp

    UNION ALL SORTED BY timestamp

    SELECT *
    FROM $access2
    ORDER BY timestamp
)

uids = (
    SELECT uid
    FROM $typed
    WHERE uid LIKE 'aaa.*'
)

timestamps = (
    SELECT timestamp
    FROM $typed
    WHERE uid IN $uids
)

---------------------

SELECT 'all' AS log_type, *, 1 AS test_index
FROM $all
LIMIT 2

SELECT COUNT(true) AS count, 2 AS test_index
FROM $ordered

SELECT timestamp, ctx, 3 AS test_index
FROM $ordered
LIMIT 10

SELECT COUNT(true) AS count, 4 AS test_index
FROM $all
WHERE context IN (
    '40i4qb1D7a61',
    '40i6qb1D7Cg1',
    'A0iCqb1D8Cg1',
    'A0iEqb1D80U1',
    'A0iGqb1D8Cg1',
    '90iAqb1D7Cg1',
    'I0iMqb1D70U1',
    'H0iKqb1D70U1',
    'O0iSqb1D7Os1',
    'b0iYqb1D7mI1'
)

SELECT COUNT(true) AS count, 5 AS test_index
FROM (
    SELECT timestamp
    FROM $all
)

SELECT timestamp, 6 AS test_index
FROM $all
LIMIT 10

SELECT *, 7 AS test_index
FROM $typed
ORDER BY timestamp DESC
LIMIT 10

SELECT *, 8 AS test_index
FROM $uids
WHERE uid IN $uids

SELECT *, 9 AS test_index
FROM $timestamps
WHERE timestamp IN $timestamps

SELECT payload_sz, timestamp, uid, 10 AS test_index
FROM $typed
WHERE timestamp IN $timestamps

SELECT MIN(timestamp) AS min_ts, 11 AS test_index
FROM $access

SELECT MAX(timestamp) AS max_ts, 12 AS test_index
FROM $access

SELECT COUNT(true) AS access_count, 13 AS test_index
FROM $access

SELECT COUNT(true) AS typed_count, 14 AS test_index
FROM $typed

SELECT COUNT(true) AS app_count, 15 AS test_index
FROM $app

SELECT
    MIN(timestamp) AS min_ts,
    MAX(timestamp) AS max_ts,
    16 AS test_index
FROM $app

SELECT
    anon1 AS module,
    COUNT(true) AS count,
    17 AS test_index
FROM $app
GROUP BY anon1

SELECT
    PERCENTILE(Float(profiler_exec), 0.1, 0.3, 0.5, 0.8, 0.9, 0.99, 0.999, 0.9999, 0.99999) AS p_exec,
    COUNT(true) AS count,
    MAX(Float(profiler_exec)) AS max_exec,
    MIN(Float(profiler_exec)) AS min_exec,
    MIN(timestamp) AS min_ts,
    MAX(timestamp) AS max_ts,
    18 AS test_index
FROM $access

SELECT
    status_code AS status_code,
    tvm_src AS tvm_source,
    COUNT(true) AS count,
    19 AS test_index
FROM $access
GROUP BY status_code, tvm_src
ORDER BY count DESC

SELECT request, 20 AS test_index
FROM $access
WHERE Int(status_code) = 1001
LIMIT 5

SELECT request, uid, 21 AS test_index
FROM $access
WHERE String(request) LIKE '.*uid=1.*'
LIMIT 5

SELECT
    uid,
    COUNT(true) AS count,
    22 AS test_index
FROM (
    SELECT RSUBSTR(request, 'user=[\d]+') AS uid
    FROM $access
)
WHERE uid != null
GROUP BY uid
ORDER BY count DESC
LIMIT 20
