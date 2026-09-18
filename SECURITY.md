# Security Policy

## Supported versions

Security fixes are applied to main first.

| Version | Supported |
| --- | --- |
| main | Yes |
| Released versions | Not yet applicable |

## Reporting a vulnerability

Do not report security vulnerabilities in public issues.

Use GitHub private vulnerability reporting when available. Include a clear description, affected component, reproduction steps, expected/observed behavior, impact, and affected commit/tag/build when known.

Do not include real secrets, personal data, credentials, or destructive proof-of-concept material.

## Scope

Relevant areas include runtime/resource handling, asset and shader parsing, rendering backends, Android/platform integration, build/CI configuration, serialization, filesystem, and future network-facing code.

## Mobile and Termux security

Never commit Android signing keys, keystores, API tokens, device identifiers, or local absolute paths. Treat asset and shader input as untrusted at load boundaries. Assets must not trigger shell commands or executable-memory behavior.

Termux build scripts must fail closed on missing tools and must not download or execute arbitrary remote scripts.

## Mobile and Termux security

- Never commit Android signing keys, keystores, credentials, API tokens, device identifiers, or local absolute paths.
- Treat assets, shaders, serialized data, and configuration as untrusted at load boundaries.
- Termux build helpers must fail closed on missing tools and must not download or execute arbitrary remote scripts.
- Validate native buffer sizes, counts, indices, handles, and arithmetic before backend calls.
- Make JNI/EGL ownership and thread shutdown ordering explicit.

## Development practices

- Validate untrusted input at subsystem boundaries.
- Avoid unchecked sizes, integer overflow, and out-of-bounds access.
- Keep dependencies and GitHub Actions dependencies current.
- Never commit credentials, private keys, access tokens, or device-specific secrets.
- Add regression tests for security-sensitive fixes.
- Prefer deterministic, reproducible builds and tests.
