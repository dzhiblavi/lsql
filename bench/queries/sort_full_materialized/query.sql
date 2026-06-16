input = input.1.txt

sorted = MATERIALIZE (
    SELECT uid, latency
    FROM $input
    ORDER BY latency, uid
)

SELECT COUNT(*) AS count
FROM $sorted
