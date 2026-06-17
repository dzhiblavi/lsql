access_1 = MATERIALIZE(STREAM 'printf "[2026-May-06 12:00:00.000000] context_id=c1 status_code=500\n[2026-May-06 12:00:01.000000] context_id=c2 status_code=200\n"')
app_1 = MATERIALIZE(STREAM 'printf "[2026-May-06 12:00:00.000000] context_id=c1 message=boom\n[2026-May-06 12:00:01.000000] context_id=c3 message=other\n"')

access = $access_1
app = $app_1

contexts_500 = (
    SELECT context_id
    FROM $access
    WHERE status_code = '500'
)

SELECT timestamp, message, context_id
FROM $app
WHERE context_id IN $contexts_500
ORDER BY context_id
