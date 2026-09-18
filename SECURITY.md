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

## Mobile and build security\n\n- Treat Android/Termux asset, shader, and configuration inputs as untrusted.\n- Do not execute shell commands or native code from assets.\n- Do not commit APK signing material, keystores, SDK credentials, or device identifiers.\n- Keep CI permissions least-privilege and avoid workflow tokens with write access unless a job explicitly requires it.\n\nSecurity development practices

Contributors should:

- Validate untrusted input at subsystem boundaries.
- Avoid unchecked buffer sizes, integer overflow, and out-of-bounds access.
- Keep third-party dependencies and GitHub Actions dependencies up to date.
- Never commit credentials, private keys, access tokens, keystores, or device-specific secrets.
- Treat Android assets, shaders, package inputs, and native file paths as untrusted at load boundaries.
- Add regression tests for security-sensitive fixes where practical.
- Prefer deterministic, reproducible builds and tests.
- Do not paste private vulnerability details into public issues or pull requests.

### Mobile and Termux

Termux is a development environment, not a trust boundary. Do not assume files accessible to a Termux process are safe input. Android builds must not embed signing credentials or local machine paths. Native code must validate sizes, offsets, handles, and external data before use.


Contributors should:

- Validate untrusted input at subsystem boundaries.
- Avoid unchecked buffer sizes, integer overflow, and out-of-bounds access.
- Keep third-party dependencies and GitHub Actions dependencies up to date.
- Never commit credentials, private keys, access tokens, or device-specific secrets.
- Add regression tests for security-sensitive fixes where practical.
- Prefer deterministic, reproducible builds and tests.
