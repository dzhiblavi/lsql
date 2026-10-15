SELECT uid FROM input.txt
WHERE uid IN (
    SELECT uid FROM input.txt
    WHERE uid IN (
        SELECT uid, status FROM input.txt
    )
)
