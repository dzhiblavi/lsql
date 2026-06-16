from lib.generate import random, random_int, write_imap_log


rows = 1_300_000

fields = {
    "bucket": random(["a", "b", "c", "d", "e", "f", "g", "h"]),
    "latency": random_int(0, 10_000),
}

write_imap_log("input.1.txt", rows, fields)
