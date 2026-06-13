from lib.generate import (
    fixed,
    format_string,
    random,
    random_int,
    random_string,
    write_imap_log,
)


rows = 40_000
statuses = ["ok", "ok", "ok", "fail"]

fields = {
    "uid": format_string("user{user}", user=random_int(0, 49_999)),
    "status": random(statuses),
    "latency": random_int(0, 5_000),
    **{f"extra{i}": random_string(distr=fixed(48)) for i in range(24)},
}

write_imap_log("input.1.txt", rows, fields)
