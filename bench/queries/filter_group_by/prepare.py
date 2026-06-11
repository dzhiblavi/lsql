from pathlib import Path


rows = 200_000
statuses = ["200", "200", "200", "404", "500", "503"]
path = Path("input.1.txt")

with path.open("w") as f:
    for i in range(rows):
        status = statuses[i % len(statuses)]
        f.write(
            f"[2026-May-06 12:00:{i % 60:02d}.123456] "
            f"context_id=ctx{i % 30000} status_code={status} "
            f"method=GET path=/api/v1/item/{i % 1000} total_time={i % 5000}\n"
        )
