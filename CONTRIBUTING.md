# Contributing to StormEngine

## Scope

StormEngine is an active C++20 engine project. Contributions should be focused, testable, portable, and explicit about ownership, lifetime, threading, and platform boundaries.

## Requirements

- C++20 compiler.
- CMake 3.20+.
- Git.
- Ninja recommended; Make is supported.
- Android SDK/NDK and Gradle for Android work.
- Termux users should use scripts/termux-build.sh for native validation.

## Termux validation

Install:

    pkg update
    pkg install clang cmake ninja make git

Run:

    ./scripts/termux-build.sh

For a low-memory phone, reduce parallelism:

    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

This validates the portable native engine. It does not replace Android SDK/NDK APK validation or physical-device GPU testing.

## Host validation

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/storm_sandbox

For release validation:

    cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DSTORM_BUILD_TESTS=ON
    cmake --build build-release --parallel
    ctest --test-dir build-release --output-on-failure

For sanitizer validation:

    cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_ENABLE_SANITIZERS=ON
    cmake --build build-sanitized --parallel
    ctest --test-dir build-sanitized --output-on-failure

## Android validation

For Android changes, run the Android CI-equivalent build:

    cd android
    gradle assembleDebug --no-daemon

The current APK build targets arm64-v8a, armeabi-v7a, x86_64, and x86.

Do not claim device-level behavior unless it was tested on a device or emulator. Record exact device/model, Android API, ABI, SoC/GPU, OpenGL ES version, renderer string, commit, and lifecycle observations.

## Code standards

- Use C++20 and keep public APIs portable unless a platform boundary requires otherwise.
- Preserve resource/entity lifetime invariants.
- Validate external input at subsystem boundaries.
- Check finite values, overflow, bounds, invalid handles, and destruction order.
- Avoid hidden ownership and unnecessary global state.
- Keep backend-neutral code independent of OpenGL ES types.
- Prefer RAII and deterministic cleanup.
- Add regression tests for new behavior and bug fixes.
- Keep changes warning-clean under the project's enabled warning flags.

## Rendering

Consider resource lifetime, shader/program lifetime, vertex/index bounds, render-state transitions, texture/sampler binding, Android/EGL lifecycle, context loss, surface recreation, and backend-independent validation.

## Threading

Android renderer lifecycle changes must account for SurfaceView callbacks, pause/resume, native thread startup/shutdown, JNI object lifetime, ANativeWindow reference counting, and concurrent restart requests.

Do not introduce polling loops that can busy-spin when a surface or resource is unavailable.

## Pull requests

A pull request should describe:

1. the problem;
2. implementation and invariants;
3. tests performed;
4. Linux/Termux/Android/GLES impact;
5. API/ABI/build compatibility impact;
6. known limitations and device-validation status.

Keep related changes in one coherent PR rather than creating many tiny PRs that are difficult to validate together.

Do not merge with unexplained failing checks. Wait for the complete required CI set after the final commit.

## Repository workflow

Use a dedicated feature branch. Before mutation, establish the base SHA. After each logical mutation, record the resulting commit SHA. Keep GitHub API operations sequential and avoid repeatedly downloading full workflow logs.

If a tool request fails or is interrupted, first re-read the branch/PR/commit state before retrying a mutation. Never assume a failed tool call did not commit.

## Security

Do not disclose vulnerabilities in public issues. Follow SECURITY.md.

## License

By contributing, you agree that your contribution is provided under the repository's MIT License, subject to applicable law.
