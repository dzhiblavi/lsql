from lib.generate import format_string, random, random_int, write_imap_log_gzip


rows = 400_000

fields = {
    "uid": format_string("user{user}", user=random_int(0, 50_000)),
    "status": random(["ok", "ok", "ok", "fail"]),
    "latency": random_int(0, 5_000),
}

write_imap_log_gzip("input.1.txt.gz", rows, fields)
