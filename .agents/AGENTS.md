# Rules of this repository

## Code

1. Never add comments in code.
2. Name your functions and variables properly.
3. Do not overengineer. When in doubt -- ask.
4. Keep code formatting: use `make check-format` and `make apply-format` from project root.
5. Adhere to SOLID. Especially to S.
6. Try to separate logical code blocks using empty lines where it makes sense.

## Testing

1. Do not add extra test types/unit test files unless explicitly asked to.
2. Only add new integration tests when there are no suitable ones already.
3. Always benchmark performance-related changes unless explicitly asked not to.
   Benchmark report should include queries, input shape, methodology and results.

## Committing changes

1. When asked for code changes, apply them in unstaged area.
2. Commit only the requested changes and only after being asked to.
3. Always check that implementation and tests are in sync with the documentation.

## Other

1. For open source presentation work, read `.agents/open-source-plan.md` and follow its order.
