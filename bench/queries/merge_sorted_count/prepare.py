from datetime import datetime

from lib.generate import imap_timestamp, write_imap_log


rows = 200_000

fields = {}


def even_timestamp(i):
    return imap_timestamp(i * 2, start=datetime(2026, 5, 6, 12, 0, 0, 123456))


def odd_timestamp(i):
    return imap_timestamp(i * 2 + 1, start=datetime(2026, 5, 6, 12, 0, 0, 123456))


write_imap_log("input.1.txt", rows, fields, timestamp=even_timestamp)
write_imap_log("input.2.txt", rows, fields, timestamp=odd_timestamp)
