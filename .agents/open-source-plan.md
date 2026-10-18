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

Item 3 is complete and approved for commit. SQL docs now cover all 18 built-in
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
After committing item 3, present item 4: configure the GitHub description,
topics, and documentation links.

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
- [ ] **4.** Configure GitHub description, topics, and documentation links.
- [ ] **5.** Run CI on pull requests and pushes to the main branch.
- [ ] **6.** Add practical examples and explain when to use lsql.
- [ ] **7.** Add CONTRIBUTING.md.
- [ ] **8.** Add issue and pull request templates and contributor-friendly issues.
- [ ] **9.** Improve release packaging and version consistency.
- [ ] **10.** Document project maturity, limitations, and roadmap.
- [ ] **11.** Add a visual demonstration and social preview.
- [ ] **12.** Document community interaction, code of conduct, and security reporting.
- [ ] **13.** Expand distribution, documentation hosting, and outreach.

Some changes can satisfy parts of later items. Preserve the order and review
what remains when each item is reached.
