# StormEngine

StormEngine is a C++20 game-engine project targeting a unified 1D, 2D, and 3D runtime with Android support.

## Status

The engine is under active development. The current tree contains the core runtime loop, fixed-step timing, ECS, scheduler, 1D/2D/3D math primitives, transforms, cameras, AABB foundations, scene/world transforms, backend-neutral rendering types, a Null renderer, an OpenGL ES backend, SpriteBatch CPU geometry, generation-safe resource lifetime, and a render-state cache.

The project is **not yet a complete 1D/2D/3D engine**. The remaining work is tracked in [Issue #9](https://github.com/thedeverfankhajo-bot/StormEngine/issues/9).

## Implemented

- C++20 core runtime and fixed-step game clock.
- ECS with sparse-set storage, queries, phases, dependencies, and deterministic scheduling.
- 1D, 2D, and 3D vector/matrix math.
- Quaternion and 3D transform support.
- Perspective and orthographic projection.
- 1D, 2D, and 3D AABB primitives.
- 1D, 2D, and 3D camera foundations.
- Parent/child world transforms with cached revisions.
- Backend-neutral render resources and draw commands.
- Null render-device validation.
- OpenGL ES texture, shader, VAO, and draw foundations.
- CPU-side SpriteBatch geometry generation.
- Generation-safe resource lifetime primitives.
- Backend-independent render-state caching.
- Linux, Android, and CodeQL GitHub Actions checks.
- Automated CTest regression coverage.

## In development

- Generation-safe renderer resource managers for buffers, textures, shaders, materials, and meshes.
- GLES uniform and texture-binding caches.
- Render graph and render passes.
- GPU SpriteBatch upload and draw.
- Complete 2D renderer.
- Complete 3D mesh/material/camera/light renderer.
- 1D/2D/3D physics bodies, broad phase, narrow phase, and solvers.
- Input and platform events.
- Asset/resource pipeline.
- Animation and audio.
- Android lifecycle and EGL surface recreation.
- Integration, sanitizer, and performance tests.
- Editor and development tooling.

## Build

Requirements:

- C++20 compiler.
- CMake 3.20 or newer.
- Git.
- Android SDK/NDK and the repository's Gradle tooling for Android builds.

Host build and tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/storm_sandbox
```

GLES configuration:

```bash
cmake -S . -B build-gles -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_BUILD_GLES=ON
cmake --build build-gles --parallel
```

## Repository layout

```text
engine/    engine source
android/   Android build and native smoke test
examples/  example applications
tests/     automated tests
.github/   CI and repository policy
```

## CI

Pull requests and pushes to `main` run:

- Linux CMake build and CTest suite.
- Android debug APK build.
- C++ CodeQL analysis.

The Linux workflow publishes debug build artifacts.

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for build, testing, coding, rendering, security, and pull-request requirements.

Security reports must follow [SECURITY.md](SECURITY.md), not public issues.

## License

StormEngine is licensed under the [MIT License](LICENSE).
