from lib.generate import format_string, random_int, write_imap_log


rows = 400_000

fields = {
    "uid": format_string("user{user}", user=random_int(0, 50_000)),
    "latency": random_int(0, 1_000_000),
    "status": "ok",
}

write_imap_log("input.1.txt", rows, fields)
