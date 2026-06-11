import gzip


LOG = """\
[2026-May-06 12:00:01.123456] uid=aaa user=alice status=ok action=login
[2026-May-06 12:00:02.654321] uid=bbb user=bob status=fail action=logout
"""


with gzip.open("input.1.txt.gz", "wt") as f:
    f.write(LOG)
