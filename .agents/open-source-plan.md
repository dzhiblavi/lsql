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

Item 1.5 is complete and approved for commit. `docs/debugging.md` contains
actual AST, bound AST, unoptimized and optimized IR, optimizer report, and
execution plan output, with explanations and diagnostic command examples.
README and the CLI reference link to it. All 9 commands and 7 output examples,
links, anchors, formatting, and whitespace checks passed.

After committing item 1.5, the user authorized work on item 2: installation
instructions and a complete quick start. The README already contains a release
download link, platform selection, and first-query example. Prepare the missing
steps from downloading the archive to installing the binary and running the
example. Keep item 2 uncommitted for review and do not advance to item 3.

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
- [ ] **2.** Add installation instructions and a complete quick start.
- [ ] **3.** Reconcile documentation with the implementation and document build prerequisites.
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
