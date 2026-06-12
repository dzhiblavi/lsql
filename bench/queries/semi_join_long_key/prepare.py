from lib.generate import fixed, random, random_int, random_string, write_imap_log


rows = 120_000
context = random_string(distr=fixed(96))

fields = {
    "context_id": context,
    "status": random(["ok", "ok", "ok", "fail"]),
    "latency": random_int(0, 10_000),
}

write_imap_log("input.1.txt", rows, fields)
