# StormEngine

StormEngine is a C++20 game-engine project building toward a unified 1D, 2D, and 3D runtime with an Android/OpenGL ES backend.

## Status

The engine is under active development and is **not yet a complete 1D/2D/3D engine**. The remaining runtime roadmap is tracked in Issue #9.

The current foundation includes:

- C++20 core runtime and fixed-step game clock.
- ECS, scheduler, dimensional math, transforms, cameras, and AABB foundations.
- Parent/child world transforms with cached revisions.
- Backend-neutral render resources, draw commands, render-state caching, Null renderer, and OpenGL ES renderer.
- CPU SpriteBatch geometry.
- Generation-safe resource lifetime and renderer validation.
- Android SurfaceView/EGL lifecycle hardening and context-loss recovery foundations.
- Linux CTest, sanitizer, Android APK, and CodeQL CI coverage.

## Build on Linux or Termux

Requirements:

- C++20 compiler.
- CMake 3.20+.
- Git.
- Ninja is recommended; Make is supported by the Termux helper.
- Android SDK/NDK and Gradle are required only for Android APK builds.

Termux:

    pkg update
    pkg install clang cmake ninja make git
    ./scripts/termux-build.sh

The helper intentionally builds the portable native core with OpenGL ES disabled. Termux is a native development/test environment; it is not a replacement for the Android NDK toolchain.

Useful options:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh
    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh
    STORM_RUN_SANDBOX=OFF ./scripts/termux-build.sh

CMake presets are also available:

    cmake --list-presets
    cmake --preset termux-debug
    cmake --build --preset termux-debug
    ctest --preset termux-debug

## Android

The Android smoke application uses CMake, the Android NDK, EGL, and OpenGL ES 3.x.

Supported build ABIs:

- arm64-v8a
- armeabi-v7a
- x86_64
- x86

These are build targets, not blanket hardware certification. A successful APK build does not prove GPU-driver compatibility.

The representative validation matrix covers Samsung Galaxy, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola families. Exact model, Android API, ABI, SoC/GPU, renderer string, commit, and lifecycle results must be recorded for physical-device validation.

See:

- docs/MOBILE_COMPATIBILITY.md
- .github/TERMUX.md
- CONTRIBUTING.md
- SECURITY.md

## Build and test

Host:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/storm_sandbox

Sanitizers:

    cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_ENABLE_SANITIZERS=ON
    cmake --build build-sanitized --parallel
    ctest --test-dir build-sanitized --output-on-failure

Android:

    cd android
    gradle assembleDebug

The Android CI verifies that generated debug APKs contain the configured native ABIs.

## Repository layout

    engine/    engine source
    android/   Android build and native smoke test
    examples/  example applications
    tests/     automated tests
    scripts/   local build helpers
    docs/      platform documentation
    .github/   CI and repository policy

## CI

Pull requests and pushes to main run native CMake/CTest, sanitizer, Android debug APK, and CodeQL validation.

CI uses workflow-level concurrency cancellation so obsolete runs on the same ref do not consume resources while newer commits are being validated.

Do not merge with unexplained failing checks.

## Development

See CONTRIBUTING.md for portability, testing, rendering, Termux, Android, and pull-request requirements.

Security vulnerabilities must follow SECURITY.md and must not be disclosed through public issues.

## License

StormEngine is licensed under the MIT License.
