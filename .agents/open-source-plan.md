# Open source presentation and discoverability

Work through the numbered items in order. Move to the next item only after
the previous item has been committed. When a commit is confirmed, present
the next item. Do not commit without the user's instruction.

## Current work

The build prerequisite is complete and approved for commit. macOS Release
was built in a fresh directory using the existing Conan cache; all 137 tests
and the formatting check passed. The user chose repeatable setup, not
byte-identical binaries.

After committing the build changes, work on item 1. Leave the README changes
uncommitted for review before advancing to item 2.

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

1. Rework README for new users.
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
