# Development and repository review process

This project is built incrementally. Before modifying code, reconstruct the current repository state rather than relying on assumptions from earlier conversations.

## Required start-of-task review

1. Read the current branch and latest commit; verify the target is `main` when working directly on the requested mainline.
2. Fetch the complete recursive repository tree, including hidden files, workflows, scripts, docs, tests, examples, and generated-file policies.
3. Read the README, architecture/roadmap/build/security/contribution docs, root build files and every file in the subsystem being changed.
4. Inspect recent commits, open issues/PRs, CI results and relevant dependency/version policy.
5. Trace public interfaces and dependencies across module boundaries. Identify callers, tests, serialization impact and platform-specific behavior before editing.
6. If a needed file or CI result cannot be read, state the gap explicitly instead of assuming it is healthy.

## Before each commit

- Keep the patch focused and explain why each added dependency or abstraction is justified.
- Add tests for behavior and regressions; test failure cases, boundary values and malformed input where relevant.
- Update docs, examples and migration notes when public behavior or build instructions change.
- Review the complete diff for accidental generated files, secrets, unrelated formatting changes and license conflicts.
- Build and run tests on available local toolchains. Use CI for the operating-system matrix; never describe unrun tests as passing.
- Inspect the tree and latest commit again after pushing. Confirm the intended files and branch were changed.

## Repository-wide re-review on every continuation

At the start of every continued work session, repeat the repository-wide tree/history/status review, then re-read the files touched in the previous step plus their callers/tests/build configuration. A prior summary is not a substitute for the current tree. Maintain a brief map of modules, decisions, unresolved risks and the exact commit last verified.

## Mainline and change safety

- Do not force-push or rewrite published history.
- Check the latest `main` SHA before writing. Use compare-and-swap/expected-SHA semantics when updating the branch so concurrent work is not silently overwritten.
- Keep commits small enough to audit; do not claim a feature is complete merely because its files exist.
- Prefer additive, reviewable changes; discuss destructive or compatibility-breaking changes before applying them.
