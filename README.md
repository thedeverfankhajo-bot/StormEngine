# StormEngine

A cross-platform game engine being built from scratch with a clean C++20 runtime and a path toward Android support.

## Current status

StormEngine is in active foundation development. The current codebase has a working core loop, ECS, dimensional math/physics primitives, world hierarchy, backend-neutral rendering abstractions, a Null renderer, and an OpenGL ES foundation. The 1D/2D/3D runtime is **not yet complete**; higher-level rendering, physics solvers, assets, input, audio, animation, and Android lifecycle integration are still being developed.

GitHub Actions continuously builds and tests the repository. Linux CI configures CMake, builds the engine and tests, runs the sandbox, and publishes debug artifacts. Android CI builds the Android smoke target. CodeQL provides static-analysis coverage.

### Implemented foundation

- Core engine identity and runtime loop
- Fixed-step game clock with frame-delta clamping
- Entity Component System with generation-safe entities
- Sparse-set component storage
- ECS queries and swap-back storage
- System phases, dependency graph, and deterministic scheduler
- 1D/2D/3D vector and matrix math foundation
- Quaternions and 3D transforms
- Perspective and orthographic projection
- 1D and 2D/3D AABB collision primitives
- 1D/2D camera foundation
- World/scene hierarchy with parent relationships and world transforms
- Backend-neutral render resource handles and draw commands
- Null render-device validation
- OpenGL ES texture/shader/VAO foundation
- Mesh/material/shader resource foundations
- Automated C++ regression tests
- CMake build and GitHub Actions CI

### In development

- Generation-safe GPU/resource lifetime management
- Render state cache and render graph
- Backend-independent sprite batching, followed by GPU sprite submission
- Complete 2D renderer
- 3D mesh/material/camera/light renderer
- 1D/2D/3D physics bodies, broad phase, narrow phase, and solver
- Transform dirty propagation and cached world matrices
- Input and platform-event abstraction
- Asset/resource pipeline
- Animation and audio
- Android surface/lifecycle-aware runtime
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
examples/ Example projects
android/  Android runtime/smoke-test project
tests/    Automated tests
docs/     Architecture and API documentation when present
.github/  CI, contribution, and repository policy configuration
```

## GitHub Actions

Open the **Actions** tab to inspect the latest Linux, Android, and CodeQL workflows. Successful Linux runs publish a `stormengine-linux-debug` artifact containing the sandbox executable and test executables.

The CI environment is a reproducible reference build. Android/Termux support is being added as the platform layer matures.

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
