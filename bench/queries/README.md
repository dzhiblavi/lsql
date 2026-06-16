# Benchmark Queries

Each directory contains one benchmark workload. `prepare.py` generates the input
shape, `query.sql` is the measured query, and `meta.json` describes what the
benchmark is intended to stress.

Benchmark target bands are intentionally category-specific. Pure scan and source
benchmarks should stay shorter than CPU-heavy benchmarks, otherwise they mostly
measure kernel and page-cache behavior.

```text
scan/io          50-200ms
gzip/io          100-500ms
group/hash       300-700ms
regex            300-700ms
sort             300-700ms
join/materialize 300-700ms
merge            300-700ms
projection       300-700ms
```

## Calibration

After running benchmarks, use the calibration helper to print row-count
suggestions based on each workload's target band:

```sh
python3 bench/calibrate.py bench/results/<tag>
```

The script only prints suggested edits. Apply them deliberately, then rerun the
suite and compare again.

## Repeat Count

By default, `bench/run.py` warms up, runs each query once as a pilot sample, and
chooses the recorded sample count from `--time-limit`:

```sh
python3 bench/run.py --warmup 3 --time-limit 5
```

Pass `--repeat N` to force an exact recorded sample count instead.

## Profiling One Workload

To dump profiles for one benchmark instead of the whole suite:

```sh
python3 bench/profile.py regex_extract --warmup 3
```

This writes the usual result JSON plus `prof.dot`, folded flamegraph files, and
captured profile output under that result directory.
