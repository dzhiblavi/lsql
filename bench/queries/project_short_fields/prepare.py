from lib.generate import format_string, random, random_int, write_imap_log


rows = 1_200_000
statuses = ["ok", "ok", "ok", "fail"]

fields = {
    "uid": format_string("user{user}", user=random_int(0, 49_999)),
    "status": random(statuses),
    "action": random(["login", "logout", "search", "update"]),
    "latency": random_int(0, 5_000),
}

write_imap_log("input.1.txt", rows, fields)
