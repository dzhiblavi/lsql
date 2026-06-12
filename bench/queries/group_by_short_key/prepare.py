from lib.generate import random, random_int, write_imap_log


rows = 200_000

fields = {
    "bucket": random(["a", "b", "c", "d", "e", "f", "g", "h"]),
    "latency": random_int(0, 10_000),
    "status": random(["ok", "ok", "fail"]),
}

write_imap_log("input.1.txt", rows, fields)
