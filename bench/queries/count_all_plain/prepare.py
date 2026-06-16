from lib.generate import write_imap_log


rows = 400_000

fields = {}

write_imap_log("input.1.txt", rows, fields)
