from lib.generate import format_string, random, random_int, write_imap_log_gzip


rows = 300_000
statuses = ["200", "200", "200", "404", "500", "503"]

fields = {
    "context_id": format_string("ctx{context}", context=random_int(0, 30_000)),
    "status_code": random(statuses),
    "total_time": random_int(0, 5_000),
}

write_imap_log_gzip("input.1.txt.gz", rows, fields)
