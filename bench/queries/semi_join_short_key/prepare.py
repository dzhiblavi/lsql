from lib.generate import format_string, random, random_int, write_imap_log


rows = 180_000

fields = {
    "uid": format_string("u{user}", user=random_int(0, 20_000)),
    "status": random(["ok", "ok", "ok", "fail"]),
    "latency": random_int(0, 10_000),
}

write_imap_log("input.1.txt", rows, fields)
