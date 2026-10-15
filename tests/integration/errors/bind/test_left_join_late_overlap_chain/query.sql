source = input.txt
left_input = (SELECT *, 1 AS left_key FROM $source)
middle_input = (SELECT 2 AS middle_key FROM (1))
right_input = (SELECT *, 3 AS right_key FROM $source)
joined = $left_input LEFT JOIN $middle_input ON left_key = middle_key
LEFT JOIN $right_input ON middle_key = right_key

SELECT shared FROM $source
