from pathlib import Path
import gzip


source = Path("input.jsonl").read_bytes()
Path("input-crlf.jsonl").write_bytes(source.replace(b"\n", b"\r\n"))
Path("input.jsonl.gz").write_bytes(gzip.compress(source))
