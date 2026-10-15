source = input.txt
left_input = (SELECT *, 1 AS left_key FROM $source)
right_input = (SELECT *, 1 AS right_key FROM $source)
joined = $left_input LEFT JOIN $right_input ON left_key = right_key

SELECT left_key FROM $joined
SELECT shared FROM $source
