from lib.generate import fixed, random, random_string, write_imap_log


rows = 200_000
context = random_string(distr=fixed(96))

fields = {
    "context_id": context,
    "status": random(["ok", "ok", "ok", "fail"]),
}

write_imap_log("input.1.txt", rows, fields)
