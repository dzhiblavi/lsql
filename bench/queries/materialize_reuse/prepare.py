from lib.generate import format_string, random, random_int, write_imap_log


rows = 300_000

fields = {
    "uid": format_string("user{user}", user=random_int(0, 40_000)),
    "status": random(["ok", "ok", "ok", "fail"]),
}

write_imap_log("input.1.txt", rows, fields)
