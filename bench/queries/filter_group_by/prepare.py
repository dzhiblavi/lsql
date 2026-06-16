from lib.generate import random, random_int, write_imap_log


rows = 2_200_000
statuses = ["200", "200", "200", "404", "500", "503"]

fields = {
    "status_code": random(statuses),
    "total_time": random_int(0, 5_000),
}

write_imap_log("input.1.txt", rows, fields)
