# StormEngine

StormEngine is an early-stage, mobile-only game-engine project targeting **Android and iOS**. Its intended game models are 1D, 2D, and 3D. The editor is also intended to run only on mobile devices.

> **Current status: Foundation bootstrap.** This repository does not yet contain a mobile runtime, renderer, physics engine, editor, asset importer, or Android/iOS export pipeline. The current code is a small, host-testable C++ foundation and must not be mistaken for a usable game engine.

## Product constraints

- Runtime targets: Android and iOS only.
- Editor targets: Android and iOS only.
- Host operating systems may be used for engineering, CI, and tests; they are not product targets.
- All repository development is performed on `main`.
- Platform and graphics capabilities are reported only after implementation and validation on real target devices.

See [Mobile-only policy](docs/MOBILE-ONLY-POLICY.md), [Architecture](docs/ARCHITECTURE.md), and [Roadmap](docs/ROADMAP.md).

## Current foundation

- C++20 and CMake project configuration.
- Typed `Error` and `Result<T>` value types.
- A platform-neutral, injectable logging interface.
- An atomic byte-budget accounting utility. It accounts for reservations; it does not allocate memory.
- A minimal engine initialization/shutdown state model. It does not start a game loop or initialize an operating-system runtime.
- A small dependency-free host test executable.

All of these are foundation-level capabilities only. No Android or iOS compatibility is claimed yet.

## Build and test the foundation

Requirements: CMake 3.24 or newer and a C++20 compiler.

```sh
cmake -S . -B build -DSTORMENGINE_BUILD_TESTS=ON
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```

Host-side test binaries are engineering tools only; they are not supported StormEngine runtime targets or editor targets.

## Architecture direction

The engine core will remain independent of editor UI and platform rendering APIs. Android and iOS integrations will sit behind explicit platform interfaces. Rendering backends will be selected based on the capabilities of each mobile platform and device; no single graphics API is assumed to be universally available.

## License

No license has been selected or added yet. Until a license is approved and committed, do not assume that this repository grants permission to reuse or redistribute its contents.
