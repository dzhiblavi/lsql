# Open source presentation and discoverability

Work through the numbered items in order. Move to the next item only after
the previous item has been committed. When a commit is confirmed, present
the next item. Do not commit without the user's instruction.

## Current work

The build prerequisite was committed as `dee17dd` (Make macOS builds repeatable
and organize platform build files). All 137 tests and the formatting check
passed. The user chose repeatable setup, not byte-identical binaries.

Item 1 is complete and approved for commit. README follows the agreed structure;
CLI and I/O details moved to `docs/cli.md` and `docs/io.md`, and the engine
description moved to `docs/dev.md`. A lossless-content audit restored the
original filtering example, the five-pass optimizer default, and the source-build
binary paths. Outdated CLI names and log-level casing were corrected.
Both examples, documentation links, formatting, and diff whitespace checks passed.
The README example was also verified against the published macOS v0.0.4 binary.

Current next item: 2, installation instructions and a complete quick start.
Present it after committing item 1. Its release download link, platform selection,
and first-query example are already covered by README; review any remaining
installation steps with the user before expanding the scope.

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

1. Rework README for new users. Completed.
2. Add installation instructions and a complete quick start.
3. Reconcile documentation with the implementation and document build prerequisites.
4. Configure GitHub description, topics, and documentation links.
5. Run CI on pull requests and pushes to the main branch.
6. Add practical examples and explain when to use lsql.
7. Add CONTRIBUTING.md.
8. Add issue and pull request templates and contributor-friendly issues.
9. Improve release packaging and version consistency.
10. Document project maturity, limitations, and roadmap.
11. Add a visual demonstration and social preview.
12. Document community interaction, code of conduct, and security reporting.
13. Expand distribution, documentation hosting, and outreach.

Some changes can satisfy parts of later items. Preserve the order and review
what remains when each item is reached.
