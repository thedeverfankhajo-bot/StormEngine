# Security policy

StormEngine is an early-stage engine project. Its current code does not yet parse game assets, load plugins, execute gameplay scripts, open network connections or package games. Security claims must track implemented and tested controls, not the roadmap.

## Supported versions

Until a versioned release process exists, security fixes are applied to the current `main` branch. No stable release branch or long-term support policy is currently declared.

## Reporting a vulnerability

Please do not publish exploit details, weaponized files or unreleased vulnerability information in a public issue. Use GitHub's private vulnerability reporting or Security Advisories feature for this repository when available. If private reporting is unavailable, contact the repository maintainer through a non-public channel before disclosing details publicly.

A useful report includes the affected commit, platform/compiler, reproducible steps, security impact and a minimal proof of concept that does not expose unrelated data.

## Automated supply-chain maintenance

Dependabot version updates are configured weekly for GitHub Actions references in
`.github/dependabot.yml`. Review these pull requests like code changes: preserve immutable
action SHA pins, verify the upstream release/version comment, and keep workflow permissions
minimal.

This configuration does **not** enable Dependabot alerts, the dependency graph, or automatic
security-update pull requests by itself. Those controls are managed separately in repository or
organization security settings. Verify their status in GitHub before advertising security
monitoring as fully enabled. As package-manager manifests or third-party runtime dependencies
are introduced, add only the relevant ecosystem configurations and review their lockfiles,
licenses, transitive dependencies, and update policy.

## Security engineering principles

- Treat scenes, project files, imported media and downloaded packages as untrusted input.
- Bound file size, item counts, nesting depth, offsets and decompression before allocating or parsing.
- Use explicit ownership and RAII; avoid unchecked pointer arithmetic and unsafe memory operations.
- Generate UUIDv4 values with operating-system cryptographic random APIs, check failures, and never silently downgrade to timestamps or a non-cryptographic pseudo-random generator.
- Keep scripting, native plugins and asset importers out of trusted runtime paths unless their trust boundary is explicit.
- Minimize third-party dependencies; track versions, licenses, advisories and update ownership.
- Give CI the minimum permissions required and pin third-party workflow actions to reviewed immutable commit SHAs.
- Never store tokens, signing keys, keystores, passwords, crash dumps containing private data or machine-specific SDK paths in the repository.
- Add fuzzing and malformed-input tests when untrusted parsers are introduced.
- Publish platform/API support and known limitations precisely. Do not claim certification or security compliance that has not been assessed.
