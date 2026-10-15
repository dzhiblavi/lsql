Hardening tests run the checked-in SQL files with `-j 16` and the default optimizations.
Each file has 32 independent STREAM inputs. `producer.py` emits log records with
unique IDs, repeated sort keys, and long string payloads. Merge inputs are already
sorted; the queries do not use ORDER BY.

- `test_union_parallel/query.sql`: balanced UNION tree.
- `test_merge_sorted_parallel/query.sql`: right-deep descending merge tree, including equal keys.
- `test_union_parallel_limit/early.sql`: left-deep UNION tree with early termination.
- `test_union_parallel_limit/query.sql`: the same tree shared by a full consumer and three limited consumers.
- `test_merge_sorted_parallel_limit/early.sql`: balanced descending merge with early termination.
- `test_merge_sorted_parallel_limit/query.sql`: balanced ascending merge shared by full and limited consumers.

Each SQL file runs four times, registered separately in `tests/integration/CMakeLists.txt`:
equally sized inputs (2048 rows each), two runs with empty, single-row, and longer
inputs, then all-empty inputs. Seeds change batch
sizes, pauses, initial delays, and EOF delays. There is no synchronization barrier
between producers. The merge LIMIT queries sort by `(key, id)` to give an exact
expected prefix even when keys are equal.

`run.py` runs exactly one SQL file with an explicit profile, seed, sort order, and
expected consumers. It passes profile and seed to the producer through environment
variables. It checks complete record multisets, merge order, limited results, process
completion, and absence of leftover STREAM processes. A CLI run has a 30-second
timeout; failures kill the process group and preserve SQL, producer, parameters,
stdout, and stderr in the test's build directory.

Run all 24 hardening tests:

```
ctest --test-dir target/armv8-macos/Debug -L hardening --output-on-failure
```

Run one profile and seed:

```
ctest --test-dir target/armv8-macos/Debug -R '^integration/hardening/test_union_parallel_query_uneven_seed1_sql$' --output-on-failure
```

Run a query manually from its directory, copying `../producer.py` there first:

```
LSQL_HARDENING_PROFILE=uneven LSQL_HARDENING_SEED=1 lsql query.sql -f JSON -j 16
```

The default profile is `balanced` and seed is `0`. The producer also accepts explicit
`--rows`, `--batch`, `--pause-ms`, `--start-ms`, `--eof-ms`, and `--descending` arguments.
