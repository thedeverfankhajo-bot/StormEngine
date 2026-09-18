# Security Policy

## Supported versions

Security fixes are applied to the default development branch first.

| Version | Supported |
| --- | --- |
| main | Yes |
| Released versions | Not yet applicable |

## Reporting a vulnerability

Do **not** report security vulnerabilities in public issues, pull requests, or discussions.

Use GitHub private vulnerability reporting when it is enabled for the repository. If private reporting is unavailable, contact the repository maintainers through a private GitHub channel and do not disclose the vulnerability publicly before a fix is available.

Include, when safe:

- affected component and commit/tag;
- concise vulnerability description;
- minimal reproduction steps;
- expected and observed behavior;
- security impact;
- relevant platform, ABI, or build configuration.

Do not include real credentials, API tokens, private keys, personal data, device identifiers, or destructive proof-of-concept material.

## Scope

Security-sensitive areas include:

- runtime and resource lifetime handling;
- ECS and bounds/overflow validation;
- asset, shader, and serialization input;
- rendering backends;
- Android/EGL/platform integration;
- build and CI configuration;
- filesystem and future network-facing code.

## Mobile and Termux security

Never commit:

- Android signing keys or keystores;
- API tokens or credentials;
- device identifiers;
- local absolute paths containing private information.

Treat assets and shader source as untrusted input at load boundaries. Asset processing must not execute shell commands or turn untrusted data into executable code.

Termux scripts must fail closed when required tools are missing and must not download or execute arbitrary remote scripts.

Android native code must validate JNI/native handles, surface lifetime, buffer sizes, integer conversions, and GPU resource lifetime.

Rotation, pause/resume, surface recreation, and EGL context loss are explicit lifecycle boundaries and must be tested as part of Android changes.

## Development practices

- Validate untrusted input at subsystem boundaries.
- Check finite values, integer overflow, sizes, indices, and handle generations.
- Preserve explicit ownership and destruction order.
- Keep backend-neutral code independent of OpenGL ES types.
- Add deterministic regression tests for security-sensitive fixes.
- Keep CodeQL analysis results published to code scanning when advanced CodeQL setup is enabled.
- Prefer reproducible builds and pinned/maintained CI actions.
- Keep dependency and toolchain versions documented and reviewable.
