# StormEngine

StormEngine is a C++20 game-engine project building toward a unified 1D, 2D, and 3D runtime with an Android/OpenGL ES backend.

## Status

The engine is under active development and is **not yet a complete 1D/2D/3D engine**. The remaining runtime roadmap is tracked in Issue #9.

Current foundations include:

- C++20 core runtime and bounded fixed-step game clock.
- ECS, scheduler, dimensional math, transforms, cameras, and AABB foundations.
- Parent/child world transforms with lazy cache invalidation.
- Backend-neutral render resources, draw commands, render-state caching, Null renderer, and OpenGL ES renderer.
- CPU SpriteBatch geometry.
- Generation-safe resource lifetime and renderer validation.
- Android SurfaceView/EGL lifecycle hardening and context-loss recovery foundations.
- Linux CTest, Release, sanitizer, Android APK/lint, and CodeQL CI coverage.

## Build on Linux or Termux

Requirements:

- C++20 compiler.
- CMake 3.20+ for direct builds and the repository's shared presets.
- Git.
- Ninja is recommended; Make is supported by the Termux helper.
- Android SDK/NDK and Gradle are required only for Android APK builds.

Termux:

    pkg update
    pkg install clang cmake ninja make git
    ./scripts/termux-build.sh

Useful options:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh
    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh
    STORM_RUN_SANDBOX=OFF ./scripts/termux-build.sh
    STORM_ENABLE_SANITIZERS=ON ./scripts/termux-build.sh

**Termux native builds intentionally keep `STORM_BUILD_GLES=OFF`.** The GLES backend requires Android/NDK; native Termux now rejects GLES mode early instead of producing a misleading host-toolchain error.

Every CTest executable has a 30-second timeout so a hung regression test cannot consume a CI runner indefinitely.

CMake presets:

    cmake --list-presets
    cmake --preset host-debug
    cmake --build --preset host-debug
    ctest --preset host-debug

CMakePresets.json uses schema version 2; build and test presets are supported starting with CMake 3.20.

## Android

The Android smoke application uses CMake, the Android NDK, EGL, and OpenGL ES 3.x.

The Android build currently targets API 36 and four ABIs:

- arm64-v8a
- armeabi-v7a
- x86_64
- x86

Android 16/API 36 is the current Google Play target floor for new apps and updates from August 31, 2026.

The project uses Android Gradle Plugin 9.4 with Gradle 9.6 and JDK 17. AGP 9.4 supports API 37, so API 36 is within the supported toolchain range.

These ABI entries are build targets, not blanket hardware certification. Runtime compatibility must be validated on physical devices.

The representative validation matrix covers five Android device families:

1. Samsung Galaxy
2. Xiaomi / Redmi / POCO
3. Google Pixel
4. OnePlus
5. Motorola

An entry for a family does not certify every model.

## Android validation

Record for every physical-device run:

- exact model;
- Android/API level;
- ABI;
- SoC/GPU;
- OpenGL ES version;
- GL vendor/renderer;
- StormEngine commit;
- surface creation/destruction;
- pause/resume;
- rotation/configuration changes;
- context-loss/recreation;
- shader compilation;
- texture upload;
- buffer lifetime;
- basic 3D rendering;
- sustained stability and visible driver errors.

The Android NDK documentation requires runtime probing of the actual OpenGL ES version/capabilities rather than assuming support from the presence of the library; the manifest declares the required GLES feature.

## Build and test

Host debug:

    cmake --preset host-debug
    cmake --build --preset host-debug
    ctest --preset host-debug
    ./build/host-debug/storm_sandbox

Host release:

    cmake --preset host-release
    cmake --build --preset host-release
    ctest --preset host-release
    ./build/host-release/storm_sandbox

Sanitizers:

    cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_ENABLE_SANITIZERS=ON
    cmake --build build-sanitized --parallel
    ctest --test-dir build-sanitized --output-on-failure
    ./build-sanitized/storm_sandbox

Android:

    cd android
    gradle assembleDebug --no-daemon
    gradle lintDebug --no-daemon

## Repository layout

    engine/    engine source
    android/   Android build and native smoke test
    examples/  example applications
    tests/     automated tests
    scripts/   local build helpers
    docs/      platform documentation
    .github/   CI and repository policy

## CI

Pull requests and pushes to main run native debug/release CTest, sanitizer validation, Android debug APK/lint, and CodeQL.

Workflow concurrency cancels obsolete runs on the same ref. Repository mutations are intentionally kept sequential during maintenance. After every write, re-read the branch/file SHA before issuing another write. If an API call is interrupted, do not retry blindly: first inspect the branch and file SHA to determine whether the write already landed.

Do not merge with unexplained failing checks.

## Mobile validation matrix

Android CI packages all four supported ABIs: `arm64-v8a`, `armeabi-v7a`, `x86_64`, and `x86`. This is ABI/build validation, not certification of every handset. The representative physical-device target families are Samsung Galaxy, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola; device certification requires a physical device or emulator.

Termux native validation exercises the portable core with GLES disabled. Android GLES validation uses the NDK build and runtime smoke path.

## Engine architecture roadmap

StormEngine is moving toward an Unreal Engine 5-class architecture in **capability and subsystem depth**, not by copying Unreal's implementation or APIs. The target is a modular runtime with explicit ownership and platform boundaries:

- **Core:** frame clock, task/scheduler primitives, logging, assertions, memory/resource lifetime.
- **World/ECS:** entities, components, transforms, scene/world queries, deterministic lifecycle.
- **Rendering:** backend-neutral render resources, generation-safe handles, render queue, render graph/render passes, GPU resource lifetime, 2D and 3D paths.
- **Platform:** Android/Termux/desktop abstraction, input/events, surface lifecycle, threading.
- **Content:** asset loading, shader/material/mesh pipelines, dependency tracking and validation.
- **Simulation:** 1D/2D/3D physics, collision, animation and audio integration.
- **Tooling:** profiling, diagnostics, editor-facing data APIs, reproducible packaging.

The current repository has the first layers and a GLES renderer, but it is not yet comparable to UE5 feature-for-feature. Issue #9 remains the authoritative completion roadmap. New subsystems must arrive with backend-neutral interfaces, deterministic tests, sanitizer coverage where applicable, and platform-specific integration tests before being marked complete.

## Development

See CONTRIBUTING.md for portability, testing, rendering, Termux, Android, and pull-request requirements.

Security vulnerabilities must follow SECURITY.md and must not be disclosed through public issues.

## License

StormEngine is licensed under the MIT License.


## Deep-hardening validation

StormEngine treats resource handles as generational capabilities: destroyed or exhausted handle generations are never reused as valid stale references. The renderer also keeps CPU-side resource state so Android EGL context recreation can rebuild GPU objects without changing public handles.

### Platform boundary

The native portable core builds in Termux with GLES disabled. The Android NDK build enables `STORM_BUILD_GLES`, links EGL/GLESv3, and compiles the Android EGL implementation only in that configuration. This keeps host/Termux builds independent of Android graphics libraries.

### Checkpointed maintenance

Repository maintenance is performed in small sequential commits. Before each write, the current branch head and target-file blob SHA are re-read; after each write, the new commit SHA is verified. CI is allowed to validate each recoverable checkpoint before merge. This avoids treating an interrupted tool call as proof that a mutation completed.
