# StormEngine

**StormEngine is an independent, modular game-engine project built from the ground up.**

The long-term goal is one coherent runtime and toolchain for 1D, 2D, and 3D experiences, with desktop development first and Android as a first-class target. The project remains at an early foundational stage and does not yet provide a production renderer, editor, physics engine, asset pipeline, or complete platform runtime.

## Project principles

- **Native and portable:** C++20 and CMake are the starting point; Android builds use the official Android NDK CMake toolchain.
- **Modular by design:** core types, platform services, rendering, scenes, assets, physics, scripting, and editor tooling will have explicit boundaries.
- **Correctness before claims:** tested behavior and published measurements matter more than feature checklists or unverified performance claims.
- **Secure by default:** validate imported data, minimize third-party dependencies and CI permissions, and review supply-chain and licensing risks.
- **Commercially maintainable:** versioned formats and APIs, clear documentation, reproducible builds, quality gates, and an explicit licensing decision.
- **Independent identity:** StormEngine will use its own architecture, APIs, tooling, and visual identity rather than copying proprietary implementation details from other engines.

## Current status

**Stage:** portable core / runtime lifecycle primitives.

The repository contains version metadata, 1D/2D/3D math primitives, scene dimension metadata, a bounded fixed-step clock, a shared status/error model, a validated runtime lifecycle state machine, UUID-compatible 128-bit stable identifier parsing/formatting, dependency-free severity-filtered logging dispatch with caller-provided sinks, a test executable, CMake build/install configuration, multi-platform CI, security guidance, and an architecture roadmap. These components are foundational building blocks, not claims of feature parity with mature engines.

See:
- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Building](docs/BUILDING.md)
- [Development and review process](docs/DEVELOPMENT.md)
- [Security policy](SECURITY.md)

## Quick start

Requirements: CMake 3.25 or newer and a C++20 compiler (GCC, Clang, or MSVC).

```sh
cmake -S . -B build -DSTORM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For Visual Studio multi-configuration generators, build and test with the desired configuration:

```sh
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

## Android direction

The core is intentionally platform-light. Android integration will be developed through the official NDK and CMake toolchain, then exercised on real ABIs/devices before the platform is declared supported. See [Building](docs/BUILDING.md) for the initial cross-compilation path.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before making changes. Meaningful changes should include tests, documentation updates where applicable, and a repo-wide context review.

## Licensing

A project license has **not** been selected yet. Until a license is added, do not assume this public repository grants permission to reuse, distribute, or commercialize its code. The licensing model must be chosen explicitly before external reuse or commercial distribution.
