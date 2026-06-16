from lib.generate import write_imap_log


rows = 4_400_000

fields = {}

write_imap_log("input.1.txt", rows, fields, seed=0)
write_imap_log("input.2.txt", rows, fields, seed=1)
