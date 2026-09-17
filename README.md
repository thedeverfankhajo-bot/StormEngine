# StormEngine

A cross-platform C++20 game engine being built from scratch, with a unified 1D/2D/3D runtime target and Android support.

## Current status

StormEngine is in active foundation development. The repository currently contains the core runtime loop, fixed-step clock, ECS, scheduler, dimensional math/physics primitives, world hierarchy, backend-neutral rendering abstractions, Null rendering validation, and an OpenGL ES foundation. The complete 1D/2D/3D engine is **not yet finished**.

GitHub Actions builds and tests Linux and Android targets and runs CodeQL analysis.

### Implemented foundation

- Core engine identity and runtime loop
- Fixed-step game clock with frame-delta clamping
- Generation-safe ECS entities and sparse-set component storage
- ECS queries, swap-back storage, phases, dependencies, and deterministic scheduling
- 1D/2D/3D vector and matrix math foundation
- Quaternion and 3D transform foundation
- Perspective and orthographic projection
- 1D/2D camera foundation
- 1D/2D/3D AABB collision primitives
- World/scene hierarchy with parent relationships and world transforms
- Backend-neutral render handles and draw commands
- Null render-device validation
- OpenGL ES texture/shader/VAO foundation
- Mesh/material/shader resource foundations
- Automated C++ regression tests
- CMake build and GitHub Actions CI

### In development

- Generation-safe GPU resource lifetime management
- Render state cache and render graph
- Sprite batching and GPU sprite submission
- Complete 2D renderer
- 3D mesh/material/camera/light rendering
- 1D/2D/3D physics bodies, broad phase, narrow phase, and solver
- Dirty transform propagation and cached world matrices
- Input and platform events
- Asset/resource pipeline
- Animation and audio
- Android lifecycle/surface-aware runtime
- Integration, sanitizer, and performance tests
- Editor and development tooling

## Build locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/storm_sandbox
```

For the GLES backend:

```bash
cmake -S . -B build-gles -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_BUILD_GLES=ON
cmake --build build-gles --parallel
```

## Repository layout

```text
engine/   Engine runtime modules
android/  Android runtime/smoke-test project
examples/ Example projects
tests/    Automated tests
.github/  CI, security, contribution, and repository policy configuration
```

## GitHub Actions

Use the **Actions** tab to inspect the latest Linux, Android, and CodeQL runs. Successful Linux runs publish a `stormengine-linux-debug` artifact containing the sandbox and test executables.

The repository is being kept honest about implementation status: a foundation is not described as a complete engine until its runtime path and tests actually exist.

## Contributing and project policies

- [Contributing guide](CONTRIBUTING.md) — development workflow, coding expectations, testing, and pull requests.
- [Security policy](SECURITY.md) — vulnerability reporting and security practices.
- [Code of Conduct](CODE_OF_CONDUCT.md) — collaboration expectations.
- [Support guide](SUPPORT.md) — bug reports, build problems, and feature requests.

## Goals

- Unified 1D/2D/3D engine runtime
- 2D and 3D rendering
- Physics and audio
- Asset/resource pipeline
- Android support
- Editor and tooling
- Reproducible automated tests, builds, and releases

## License

TBD
