from pathlib import Path


rows = 200_000
path = Path("input.1.txt")

with path.open("w") as f:
    for i in range(rows):
        f.write(
            f"[2026-May-06 12:00:{i % 60:02d}.123456] "
            f"uid=user{i % 10000} status=ok action=ping latency={i % 1000}\n"
        )
