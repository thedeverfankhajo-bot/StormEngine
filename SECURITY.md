# Security Policy

## Supported versions

Security fixes are applied to the default development branch first.

| Version | Supported |
| --- | --- |
| main | Yes |
| Released versions | Not yet applicable |

## Reporting a vulnerability

Do **not** report security vulnerabilities in public issues, pull requests, or discussions.

Use GitHub private vulnerability reporting when it is enabled for the repository. If private reporting is unavailable, contact the maintainers through a private GitHub channel and do not disclose the vulnerability publicly before a fix is available.

Include, when safe:

- affected component and commit/tag;
- concise vulnerability description;
- minimal reproduction steps;
- expected and observed behavior;
- security impact;
- relevant platform, ABI, or build configuration.

Never include real credentials, API tokens, private keys, personal data, device identifiers, or destructive proof-of-concept material.

## Scope

Security-sensitive areas include:

- runtime and resource lifetime handling;
- ECS bounds, stale handles, and overflow validation;
- asset, shader, and serialization input;
- rendering backends and GPU state;
- Android/JNI/EGL/platform integration;
- build and CI configuration;
- filesystem and future network-facing code.

## Mobile and Termux security

Never commit:

- Android signing keys or keystores;
- API tokens or credentials;
- device identifiers;
- local absolute paths containing private information.

Treat assets and shader source as untrusted input at load boundaries. Asset processing must not execute shell commands or turn untrusted data into executable code.

Termux scripts fail closed when required tools are missing, validate their toolchain versions/options, and do not download or execute arbitrary remote scripts.

Android native code must validate JNI/native handles, surface lifetime, buffer sizes, integer conversions, GPU resource lifetime, and context-loss recovery.

## Rendering security

Rendering entry points validate:

- resource handle generations;
- vertex/index bounds;
- vertex layout locations and offsets;
- texture dimensions and mip levels;
- integer-size conversions into GLES types;
- shader stage compatibility;
- frame lifecycle;
- context-loss state.

CPU-side resource data is retained where required for context recreation. GPU object names are never treated as durable identifiers across context loss.

## CI security and reliability

CI uses least-privilege repository permissions and workflow concurrency to cancel obsolete runs. Build scripts fail closed on unsupported platform/toolchain combinations. Do not execute untrusted pull-request data as shell code; review third-party action updates before adopting them.

## Engine architecture security

As the engine grows toward a large modular runtime, subsystem boundaries are security boundaries. New resource managers, render graphs, asset loaders, task systems, serialization, editor tooling, and platform adapters must validate ownership, sizes, generations, thread affinity, and lifetime transitions at their public boundaries. Do not assume a caller has already validated data merely because it came from another engine subsystem.

## Development practices

- Validate untrusted input at subsystem boundaries.
- Check finite values, integer overflow, sizes, indices, and handle generations.
- Preserve explicit ownership and destruction order.
- Keep backend-neutral code independent of OpenGL ES types.
- Add deterministic regression tests for security-sensitive fixes.
- Prefer reproducible builds and maintained/pinned CI actions.
- Keep dependency and toolchain versions documented and reviewable.
- Use sanitizer and static-analysis coverage before merging substantial runtime changes.
