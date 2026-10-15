import gzip
import os
from pathlib import Path


sources = {
    "empty.log": b"",
    "partial.log": b"first\nlast",
    "unterminated.log": b"last",
    "crlf.log": b"first\r\nsecond\r\n",
    "long.log": b"row0" + b"x" * 131072 + b"tail0\n" + b"row1" + b"y" * 131072 + b"tail1\n",
}

boundary = 2 * os.sysconf("SC_PAGESIZE")
sources["pages.log"] = b"".join(
    character * (length - len(suffix)) + suffix + b"\n"
    for character, suffix, length in [
        (b"a", b"end_a", boundary - 1),
        (b"b", b"end_b", boundary),
        (b"c", b"end_c", boundary),
    ]
)

for name, contents in sources.items():
    Path(name).write_bytes(contents)
    with gzip.open(name + ".gz", "wb") as output:
        output.write(contents)
