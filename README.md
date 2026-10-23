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

**Stage:** portable core / in-memory 1D, 2D and 3D scenes with version-1 binary save/load.

The repository contains version metadata, 1D/2D/3D vector primitives, a tested column-major 4×4 Matrix4 API, and `Transform::to_matrix()` conversion for every scene dimension, plus a dimension-aware in-memory Scene/Entity/Transform model, a bounded fixed-step clock, a shared status/error model, a validated runtime lifecycle state machine, UUID-compatible 128-bit stable identifiers with OS-backed version-4 generation, parsing/formatting and an in-memory duplicate-detecting registry, validated in-memory runtime configuration, dependency-free severity-filtered logging dispatch and a bounded no-heap diagnostics ring buffer, a fixed-capacity, platform-neutral digital input-state primitive with stable named engine-level `KeyCode` values, per-frame press/release edges and focus-loss release handling, a validated fixed-capacity FIFO platform-event queue with explicit overflow and rejection counters, an optional SDL3 desktop bootstrap demo, a test executable, CMake build/install configuration, multi-platform CI, security guidance, and an architecture roadmap. The scene model validates finite dimension-matched transforms, bounds and validates entity names, supports safe renaming and deterministic stable-ID-ordered visitation, owns entity storage, and uses OS-generated stable identifiers. A dependency-free version-1 SceneCodec plus native SceneFile service saves and reopens 1D/2D/3D scene records with stable IDs, a 64 MiB input/output limit, strict validation, transactional loading, and same-directory temporary-file replacement. A validated UTF-8 `ProjectPath` value now defines cross-platform logical asset-path spelling, but does not yet sandbox filesystem access. Scene hierarchy, components, project bundles, root-confined file I/O, and format migration are not implemented. These components are foundational building blocks, not claims of feature parity with mature engines. The generic input-state API remains adapter-fed and does not yet define a stable cross-platform key mapping; the optional SDL3 desktop sample provides a provisional mapping and pumps native window/input events.

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

## Optional SDL3 desktop prototype

To build the first interactive desktop slice, opt in to the SDL3-backed demo:

```sh
cmake -S . -B build-sdl3 -DSTORM_BUILD_TESTS=ON -DSTORM_BUILD_SDL3_DEMO=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sdl3 --target storm_sdl3_demo --config Debug --parallel 2
```

Run `build-sdl3/storm_sdl3_demo` on Linux/macOS, or `build-sdl3/Debug/storm_sdl3_demo.exe` for Visual Studio. Move the colored rectangle with WASD or the arrow keys, click to reposition it, and use the mouse wheel to resize it. Escape or the window close button exits.

The optional target downloads the pinned SDL3 3.4.18 release archive and verifies its SHA-256. The normal core build remains dependency-free when `STORM_BUILD_SDL3_DEMO=OFF`. This is a desktop bootstrap sample using SDL's simple 2D renderer; it is not yet StormEngine's production Render Hardware Interface, texture/sprite asset pipeline, editor, or Android APK. See [Building](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md), and [third-party notices](THIRD_PARTY_NOTICES.md).

## Android direction

The core is intentionally platform-light. Android integration will be developed through the official NDK and CMake toolchain, then exercised on real ABIs/devices before the platform is declared supported. See [Building](docs/BUILDING.md) for the initial cross-compilation path.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before making changes. Meaningful changes should include tests, documentation updates where applicable, and a repo-wide context review.

## Licensing

A project license has **not** been selected yet. Until a license is added, do not assume this public repository grants permission to reuse, distribute, or commercialize its code. The licensing model must be chosen explicitly before external reuse or commercial distribution.
