from pathlib import Path
import gzip


source = Path("input.csv").read_bytes()
Path("input-crlf.csv").write_bytes(source.replace(b"\n", b"\r\n"))
Path("input.csv.gz").write_bytes(gzip.compress(source))
