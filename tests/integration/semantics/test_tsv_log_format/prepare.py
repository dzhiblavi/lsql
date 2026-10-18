from pathlib import Path
import gzip


source = Path("input.tsv").read_bytes()
Path("input-crlf.tsv").write_bytes(source.replace(b"\n", b"\r\n"))
Path("input.tsv.gz").write_bytes(gzip.compress(source))
