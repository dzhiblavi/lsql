from datetime import datetime

from lib.generate import imap_timestamp, write_imap_log


rows = 200_000

fields = {}


def timestamp(i):
    return imap_timestamp(i, start=datetime(2026, 5, 6, 12, 0, 0, 123456))


write_imap_log("input.1.txt", rows, fields, timestamp=timestamp)
