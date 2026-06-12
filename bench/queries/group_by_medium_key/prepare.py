from lib.generate import fixed, random_int, random_string, write_imap_log


rows = 160_000

fields = {
    "bucket": random_string(distr=fixed(32)),
    "latency": random_int(0, 10_000),
}

write_imap_log("input.1.txt", rows, fields)
