input = input.1.txt

filtered_contexts = (
    SELECT context_id
    FROM $input
    WHERE status_code != '200'
)

---------------------

SELECT *, 1 AS test_index
FROM $filtered_contexts
