from lib.generate import write_imap_log


rows = 1_500_000

fields = {}

write_imap_log("input.1.txt", rows, fields)
