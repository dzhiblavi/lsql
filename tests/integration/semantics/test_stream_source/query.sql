SELECT uid, status
FROM STREAM 'printf "[2026-May-06 12:00:00.000000] uid=aaa status=ok\n[2026-May-06 12:00:01.000000] uid=bbb status=fail\n"'
WHERE status != 'fail'
