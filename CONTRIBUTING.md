# Contributing to StormEngine

Thank you for helping build StormEngine. The codebase is at an early stage, so architecture, clear reasoning and reproducible tests matter as much as implementation speed.

## Ground rules

- Read the [architecture](docs/ARCHITECTURE.md), [roadmap](docs/ROADMAP.md) and [development process](docs/DEVELOPMENT.md) before broad changes.
- Follow the repository-wide review process at the beginning of each task; do not rely on stale summaries.
- Keep the C++20 baseline until a documented decision changes it.
- Prefer standard library facilities, explicit ownership and RAII.
- Do not add third-party code before its license, transitive dependencies, platform coverage and maintenance status have been reviewed.
- Do not add credentials, local SDK paths, build artifacts, user data or untrusted binary assets to Git.
- Do not copy proprietary code or assets from other engines.

## Code and API conventions

- Keep public headers under `include/storm/` and implementation files under `src/`.
- Use the `storm` namespace, descriptive names, four-space indentation and the checked-in formatting policy.
- Use `[[nodiscard]]` on results that callers should not ignore.
- Prefer explicit error results over silent fallback for invalid state or malformed content.
- Avoid undefined behavior, owning raw pointers, unchecked external input and undocumented global state.
- Keep platform, renderer, editor and scripting dependencies behind narrow module boundaries.
- Document coordinate conventions, units, lifecycle and threading assumptions as soon as they become public API contracts.

## Tests

Tests must be deterministic and independent of GPU, wall-clock timing, network access and developer-specific paths unless specifically testing an integration boundary. Add tests for both normal and invalid/boundary inputs.

Before submitting a change, configure, build and run CTest as described in [Building](docs/BUILDING.md). State any operating system or toolchain you could not test locally. CI is expected to validate Linux, macOS and Windows.

## Commit messages

Use short, imperative, scoped messages such as `feat(core): add fixed-step clock`, `test(math): cover normalization edge cases` or `docs(android): clarify NDK setup`.

## Pull request checklist

The project uses the checklist in `.github/PULL_REQUEST_TEMPLATE.md`. Include motivation, behavior changes, testing evidence, compatibility impact and any new dependency/license considerations.

## Licensing

StormEngine's original project code and documentation are licensed under Apache-2.0; read [`LICENSE`](LICENSE) before redistributing changes. Submit only contributions you have the right to license under Apache-2.0, preserve existing copyright and attribution notices, and disclose third-party code/assets with their source and license. Do not add content with unclear or incompatible licensing. The project license does not grant permission to use StormEngine trademarks or logos. If a contribution is intentionally excluded from the Apache-2.0 contribution terms, mark it conspicuously as “Not a Contribution” when submitting it.
