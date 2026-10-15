SELECT l.anon FROM ('a') AS l LEFT JOIN ('a') AS r ON l.anon = r.anon LEFT JOIN ('a') AS l ON r.anon = l.anon
