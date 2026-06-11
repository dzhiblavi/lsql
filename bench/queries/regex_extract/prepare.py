from pathlib import Path


rows = 100_000
path = Path("input.1.txt")

with path.open("w") as f:
    for i in range(rows):
        user = i % 50_000
        session = i % 10_000
        f.write(
            f"[2026-May-06 12:00:{i % 60:02d}.123456] "
            f"uid=user{user} status=ok "
            f"request=/api/search?user={user}&session=s{session}&q=item{i % 1000}\n"
        )
