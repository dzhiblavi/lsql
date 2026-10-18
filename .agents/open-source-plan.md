# Open source presentation and discoverability

Work through the numbered items in order. Move to the next item only after
the previous item has been committed. When a commit is confirmed, present
the next item. Do not commit without the user's instruction.

## Current work

The build prerequisite was committed as `dee17dd` (Make macOS builds repeatable
and organize platform build files). All 137 tests and the formatting check
passed. The user chose repeatable setup, not byte-identical binaries.

Item 1 was committed as `b865416` (Rework README and move reference
documentation into docs). Its lossless-content audit preserved the original
example, optimizer default, source-build binary paths, and engine description.

Item 1.5 was committed as `3bfb3dd` (Document query debugging with AST, IR,
and execution plan examples). All 9 documented commands and 7 output examples,
links, anchors, formatting, and whitespace checks passed.

Item 2 was committed as `b059e23` (Document release installation and a complete
quick start). Its full workflow was checked with published v0.0.4 binaries on
macOS and in Ubuntu 20.04 amd64.

Item 3 was committed as `f03a470` (Align documentation with SQL behavior and
make COUNT case-insensitive). SQL docs now cover all 18 built-in
functions, STREAM sources, sort direction, regex matching, casts, nulls, timestamp formats, and aggregate
constraints. I/O docs clarify format detection, anonymous columns, timestamp
ordering, CSV behavior, and no-buffer restrictions. CLI docs clarify case
sensitivity, timestamp interpretation, and stack trace availability. Build
instructions remain short and now include cloning and host prerequisites.

The user explicitly requested fixing COUNT case sensitivity rather than
keeping a documented exception. COUNT now parses as an ordinary identifier;
function calls normalize its name to count_all/count_nonnull, and star calls
remain restricted to COUNT. The user accepted this design after discussing why
making lowercase count a lexer keyword would break aliases and field names.
Preserve this design. Existing aggregation fixtures cover casing, nulls,
aliases, and macros, and release notes mention the change.

The rebuilt parser passed all 137 tests. All 22 complete SQL reference examples,
all documented function names, COUNT variants and invalid calls, documentation
links and anchors, formatting, whitespace, and Linux make command expansion
were checked. No new test files or test types were added.

Item 4 is complete per the user: they added the GitHub description and topics
and asked to mark this item done.

Item 5 was committed as `aa1d534` (Update workflows). CI reuses the Linux
amd64 and macOS arm64 builds on pull requests, pushes to `main`, and manual
runs. Tests exclude hardening; Linux also checks formatting. CI uploads build
artifacts without creating tags or GitHub Releases. The separate Release
workflow is manual and accepts a `tag` input. actionlint, formatting,
whitespace, workflow structure, and documentation links passed locally.

Item 6 is current. The user requested researching alternatives and proposing
comparison criteria before deciding what value lsql offers and documenting it
in README. An honest conclusion that lsql duplicates existing tools is welcome;
do not invent differentiators or claim performance without measurements.

The user prefers a short README comparison table with brief explanations;
avoid expanding this into a large benchmark project unless requested.

A format-support detour was implemented and approved for commit: IMAP is now
TS_LOGFMT (timestamped logfmt), TSKV2 is now TSKV, and headerless CSV, TSV, and
JSONL were added. Timestamp support is optional; formats without it reject
time-range reads. TSKV detection requires a standalone tab-separated marker.
CSV handles quotes and doubled quotes; positional fields remain anon1..anon32.
JSONL root fields are nullable strings. simdjson decodes root strings and keys;
numbers and nested containers remain raw JSON text, with numeric conversion
left to lsql casts. Nested contents are not fully validated on input. The user
accepted the Log.h refactor using util::Overloaded for borrowed and owned
strings. Preserve it. README, I/O and SQL docs, and release notes were updated.
Eight integration fixtures cover the new formats, malformed records, gzip,
CRLF, STREAM, and rejection of timestamp ranges. The Release build, all 121
non-hardening tests, formatting, links, whitespace checks, and documented JSONL
example passed. This detour does not complete item 6; resume its compact
comparison table and practical use cases after committing the format changes.

Initial research used official docs and repositories for DuckDB CLI,
clickhouse-local, lnav, angle-grinder, dsq, OctoSQL, SPyQL, trdsql, q, and
Tailpipe. SQL over local files without a server is already available elsewhere.
lnav supports SQL and headless mode; dsq supports logfmt; ClickHouse supports
TSKV and extractKeyValuePairs. Generic key/value parsing is not unique.

Potential niche to validate: direct timestamped logfmt/TSKV access with little setup,
reusable SQL scripts across files, and narrow timestamp ranges in sorted logs.
Source inspection confirms binary search for timestamp boundaries followed by
reading the selected byte range; this requires ascending timestamps and
uncompressed files. Competitor behavior and relative performance remain
unmeasured.

Proposed criteria: preparation from raw input to result; exact input-format
compatibility and messy/missing fields; query expressiveness and SQL semantics;
time-range access; files/gzip/pipes/live behavior; full end-to-end latency, peak
memory and disk needs; installation footprint/platforms; diagnostics and
maintenance. Distinguish native support, configuration/adapter, import, and
unsupported cases. Compare identical results and include conversion/indexing
cost, with first-use and repeated-use results reported separately.

Representative tasks: grouped error counts; five minutes from a large sorted
log; latency percentiles; LEFT JOIN requests/responses by request ID; merging
service logs in timestamp order. Repeat representative tasks with JSONL and
compressed input so the comparison is not limited to lsql's preferred format.
No benchmarks or competitor installations have been performed yet.

Primary sources for the comparison:
- https://duckdb.org/docs/current/clients/cli/overview
- https://duckdb.org/docs/current/data/json/overview
- https://duckdb.org/community_extensions/extensions/read_lines
- https://clickhouse.com/docs/concepts/features/tools-and-utilities/clickhouse-local
- https://clickhouse.com/docs/reference/formats/TabSeparated/TSKV
- https://clickhouse.com/docs/reference/functions/regular-functions/tuple-map-functions
- https://lnav.org/features
- https://docs.lnav.org/en/latest/formats.html
- https://github.com/rcoh/angle-grinder
- https://github.com/multiprocessio/dsq
- https://github.com/cube2222/octosql
- https://github.com/dcmoura/spyql
- https://github.com/noborus/trdsql
- https://github.com/harelba/q
- https://github.com/turbot/tailpipe

Build instructions belong in `docs/build/linux.md` and
`docs/build/mac_silicon.md`. Keep them limited to prerequisites and commands,
without implementation details that would require frequent updates.
Keep platform-specific build assets under `build/linux/` and `build/macos/`.

For item 1, the user approved this README structure:

1. What the tool is and which problem it solves.
2. An example query with its output.
3. Downloading from GitHub Releases and a quick start.
4. Links to documentation for CLI flags, SQL, and building from source.

## Ordered backlog

- [x] **1.** Rework README for new users.
- [x] **1.5.** Add query debugging documentation with AST, bound AST, IR, optimizer report, and execution plan examples; link it from README.
- [x] **2.** Add installation instructions and a complete quick start.
- [x] **3.** Reconcile documentation with the implementation and document build prerequisites.
- [x] **4.** Configure GitHub description, topics, and documentation links.
- [x] **5.** Run CI on pull requests and pushes to the main branch.
- [ ] **6.** Compare alternatives, establish when to use lsql, and add practical examples and README positioning.
- [ ] **7.** Add CONTRIBUTING.md.
- [ ] **8.** Add issue and pull request templates and contributor-friendly issues.
- [ ] **9.** Improve release packaging and version consistency.
- [ ] **10.** Document project maturity, limitations, and roadmap.
- [ ] **11.** Add a visual demonstration and social preview.
- [ ] **12.** Document community interaction, code of conduct, and security reporting.
- [ ] **13.** Expand distribution, documentation hosting, and outreach.

Some changes can satisfy parts of later items. Preserve the order and review
what remains when each item is reached.
