from lib.generate import format_string, random, random_int, write_imap_log


rows = 400_000
statuses = ["200", "200", "200", "404", "500", "503"]

fields = {
    "context_id": format_string("ctx{context}", context=random_int(0, 29_999)),
    "status_code": random(statuses),
    "method": "GET",
    "path": format_string("/api/v1/item/{item}", item=random_int(0, 999)),
    "total_time": random_int(0, 5_000),
}

write_imap_log("input.1.txt", rows, fields)
