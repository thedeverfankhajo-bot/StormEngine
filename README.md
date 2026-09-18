# StormEngine

StormEngine is a C++20 game-engine project targeting a unified 1D, 2D, and 3D runtime with Android support.

## Status

The engine is under active development and is not yet a complete 1D/2D/3D engine. Remaining engine work is tracked in Issue #9.

## Implemented

- C++20 core runtime and fixed-step game clock.
- ECS, scheduler, dimensional math, transforms, cameras, AABB foundations.
- Parent/child world transforms with cached revisions.
- Backend-neutral render resources, draw commands, Null renderer, OpenGL ES backend.
- CPU SpriteBatch geometry.
- Generation-safe resource lifetime and render-state caching.
- Android lifecycle/EGL context recovery work.
- Automated CTest, sanitizer, Android, and CodeQL validation.

## Platform and Termux

The portable C++ core can be built and tested directly in Termux. Android APK construction remains an SDK/NDK/Gradle workflow.

Representative Android test families are Samsung Galaxy, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola. These are compatibility-test families, not blanket certification.

Android build targets are arm64-v8a, armeabi-v7a, x86_64, and x86. Physical-device GPU validation is separate from APK compilation.

Termux quick start:

    pkg update
    pkg install clang cmake ninja make git
    ./scripts/termux-build.sh

## Build

Requirements:

- C++20 compiler.
- CMake 3.20+.
- Git.
- Android SDK/NDK and Gradle for Android APKs.
- Termux users can use the repository script for native builds.

Host:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

GLES configuration:

    cmake -S . -B build-gles -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_BUILD_GLES=ON
    cmake --build build-gles --parallel

## Repository layout

    engine/    engine source
    android/   Android build and native smoke test
    examples/  example applications
    tests/     automated tests
    scripts/   local build helpers
    docs/      platform documentation
    .github/   CI and repository policy

## CI

Pull requests and pushes to main run Linux CMake/CTest, sanitizer, Android debug APK, and CodeQL checks.

## Development

See CONTRIBUTING.md for build, testing, rendering, security, and pull-request requirements. See docs/MOBILE_COMPATIBILITY.md for the mobile test matrix.

Security reports must follow SECURITY.md, not public issues.

## License

StormEngine is licensed under the MIT License.
