from lib.generate import write_imap_log_gzip


rows = 10_800_000

fields = {}

write_imap_log_gzip("input.1.txt.gz", rows, fields)
