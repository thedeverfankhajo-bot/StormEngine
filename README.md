# StormEngine

A cross-platform game engine being built from scratch with a focus on a clean C++20 runtime and a path toward Android support.

## Status

Early runtime architecture is in active development.

### Implemented

- Core engine identity and runtime loop
- Fixed-step game clock with frame-delta clamping
- Entity Component System with generation-safe entities
- Sparse-set component storage
- ECS queries and swap-back storage
- System phases, dependency graph, and deterministic scheduler
- 2D/3D vector and matrix math
- Quaternions and transforms
- Perspective and orthographic matrix construction
- Automated core, ECS, game-loop, scheduler, and math tests
- CMake build and GitHub Actions CI

### In progress

- Rendering abstraction and graphics backend
- Camera and scene systems
- Asset/resource pipeline
- Physics and audio modules
- Android runtime/backend
- Editor and development tooling

## Goals

- Core engine and math
- Entity Component System (ECS)
- 2D/3D rendering
- Physics and audio
- Asset/resource pipeline
- Android support
- Editor and tooling
- Automated tests and CI

## Repository Layout

```text
engine/   Engine runtime modules
editor/   Editor and development tools
tools/    Asset/build utilities
examples/ Example projects
tests/    Automated tests
docs/     Architecture and API documentation
```

## Build

```bash
cmake -S . -B build -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## License

TBD
