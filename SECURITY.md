# Security Policy

## Supported versions

StormEngine is currently in active development. Security fixes are applied to the `main` branch first.

| Version | Supported |
| --- | --- |
| `main` | Yes |
| Released versions | Not yet applicable |

## Reporting a vulnerability

Please do **not** report security vulnerabilities in public issues.

For a private report, use GitHub's private vulnerability reporting feature for this repository when available. Include:

- A clear description of the vulnerability.
- The affected component, file, or subsystem.
- Reproduction steps or a minimal proof of concept.
- The expected and observed behavior.
- The potential security impact.
- The affected commit, tag, or build when known.

Please avoid including real secrets, personal data, production credentials, or destructive proof-of-concept code in a report.

## Scope

Security reports are especially relevant to:

- Engine runtime and resource handling.
- Asset loading and parsing.
- Rendering backends and shader handling.
- Android and platform integration.
- Build and CI configuration.
- Serialization, file-system, and network-facing code added to the engine.

## Response

Reports will be reviewed by the maintainers. Once a vulnerability is confirmed, the maintainers will determine the affected versions, prepare a fix, and document the impact and remediation as appropriate.

Please allow reasonable time for investigation and remediation before publicly disclosing an unresolved vulnerability.

## Platform-specific security notes

Android and Termux builds must not embed signing keys, keystores, API tokens, device identifiers, or local absolute paths. Native asset and shader inputs should be treated as untrusted data at load boundaries. Do not enable executable-memory or shell-command behavior for assets.

## Security development practices

Contributors should:

- Validate untrusted input at subsystem boundaries.
- Avoid unchecked buffer sizes, integer overflow, and out-of-bounds access.
- Keep third-party dependencies and GitHub Actions dependencies up to date.
- Never commit credentials, private keys, access tokens, or device-specific secrets.
- Add regression tests for security-sensitive fixes where practical.
- Prefer deterministic, reproducible builds and tests.
