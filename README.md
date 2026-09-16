# StormEngine

A cross-platform game engine being built from scratch with a clean C++20 runtime and a path toward Android support.

## Current status

The repository is continuously built and tested by GitHub Actions. The CI pipeline configures the CMake project, builds the engine and tests, executes the sandbox, and publishes the Linux debug binaries as workflow artifacts.

### Implemented

- Core engine identity and runtime loop
- Fixed-step game clock with frame-delta clamping
- Entity Component System with generation-safe entities
- Sparse-set component storage
- ECS queries and swap-back storage
- System phases, dependency graph, and deterministic scheduler
- 2D/3D vector and matrix math
- Quaternions and transforms
- Perspective and orthographic projection
- Camera system
- World/scene hierarchy with parent relationships and world transforms
- Backend-neutral render resource handles and draw commands
- Automated C++ tests
- CMake build and GitHub Actions CI

### In progress

- Render device and command abstraction
- OpenGL ES backend
- Mesh/material/shader resources
- Asset/resource pipeline
- Input and platform layer
- Android runtime/backend
- Editor and development tooling

## Run the engine on GitHub

Open the **Actions** tab and select **Build, Test, and Run Sandbox**. Every successful run produces a `stormengine-linux-debug` artifact containing the sandbox executable and test executables.

Download the artifact, extract it, and run:

```bash
./storm_sandbox
./storm_render_tests
```

The CI machine is currently the first reproducible execution environment for the engine. Android/Termux execution will be added as the platform layer matures.

## Build locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/storm_sandbox
```

## Repository layout

```text
engine/   Engine runtime modules
editor/   Editor and development tools
tools/    Asset/build utilities
examples/ Example projects
tests/    Automated tests
docs/     Architecture and API documentation
.github/  CI workflows
```

## Goals

- Core engine and math
- Entity Component System (ECS)
- 2D/3D rendering
- Physics and audio
- Asset/resource pipeline
- Android support
- Editor and tooling
- Reproducible automated tests and builds

## License

TBD
