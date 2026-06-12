from lib.generate import format_string, random_int, write_imap_log


rows = 200_000

fields = {
    "uid": format_string("user{user}", user=random_int(0, 9_999)),
    "status": "ok",
    "action": "ping",
    "latency": random_int(0, 999),
}

write_imap_log("input.1.txt", rows, fields)
