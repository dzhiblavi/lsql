# Benchmark Queries

Each directory contains one benchmark workload. `prepare.py` generates the input
shape, and `query.sql` is the measured query.

## Workloads

```text
count_all_plain        sequential scan and COUNT(*) over minimal plain text lines
filter_group_by        filter, string grouping, integer cast, aggregate, small sort
group_by_short_key     low-cardinality short string grouping
group_by_medium_key    high-cardinality medium string grouping
group_by_long_key      high-cardinality long string grouping
gzip_count_all         gzip stream read and COUNT(*) over minimal lines
gzip_group_by          gzip stream read plus grouping and aggregate
project_short_fields   filtered projection of a few short fields plus top-k
project_wide_lines     filtered projection of raw wide _line values plus top-k
regex_extract          LIKE and RSUBSTR over medium request strings
regex_extract_long     LIKE and RSUBSTR over long message strings
semi_join_short_key    semi-join with short keys
semi_join_long_key     semi-join with long keys
sort_topk              top-k optimization over sort and limit
sort_full_materialized full sort whose output is materialized and consumed
time_range_count       timestamp lower/upper bound search plus range scan
union_all_count        concatenation of two file sources
merge_sorted_count     sorted union / merge over two sorted inputs
materialize_reuse      materialized filtered relation reused by later query
```
