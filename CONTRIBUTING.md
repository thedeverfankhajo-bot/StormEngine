# Contributing to StormEngine

## Scope

StormEngine is an active C++20 engine project. Contributions should be focused, testable, portable, and explicit about ownership, lifetime, threading, and platform boundaries.

## Requirements

- C++20 compiler.
- CMake 3.23+.
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

For a low-memory phone:

    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

Release:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh

GLES is an Android/NDK backend; a generic Termux native build validates the portable core rather than pretending to be an Android cross-build.

## Host validation

Debug:

    cmake --preset host-debug
    cmake --build --preset host-debug
    ctest --preset host-debug
    ./build/host-debug/storm_sandbox

Release:

    cmake --preset host-release
    cmake --build --preset host-release
    ctest --preset host-release
    ./build/host-release/storm_sandbox

Sanitizers:

    cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON -DSTORM_ENABLE_SANITIZERS=ON
    cmake --build build-sanitized --parallel
    ctest --test-dir build-sanitized --output-on-failure
    ./build-sanitized/storm_sandbox

## Android validation

For Android changes, run:

    cd android
    gradle assembleDebug --no-daemon
    gradle lintDebug --no-daemon

The APK build targets arm64-v8a, armeabi-v7a, x86_64, and x86.

Do not claim device-level behavior unless it was tested on a device or emulator. Record exact model, Android API, ABI, SoC/GPU, OpenGL ES version, renderer/vendor, commit, and lifecycle observations.

## Device matrix

The required representative Android matrix spans:

- Samsung Galaxy;
- Xiaomi / Redmi / POCO;
- Google Pixel;
- OnePlus;
- Motorola.

These are device families, not separate mobile operating systems and not blanket certification. Each result must identify the exact model and software stack.

## Code standards

- Use C++20 and keep public APIs portable unless a platform boundary requires otherwise.
- Preserve resource/entity lifetime invariants.
- Validate external input at subsystem boundaries.
- Check finite values, overflow, bounds, invalid handles, and destruction order.
- Avoid hidden ownership and unnecessary global state.
- Keep backend-neutral code independent of OpenGL ES types.
- Prefer RAII and deterministic cleanup.
- Add regression tests for new behavior and bug fixes.
- Keep changes warning-clean.

## Rendering

Review resource lifetime, shader/program lifetime, vertex/index bounds, render-state transitions, texture/mipmap behavior, Android/EGL lifecycle, surface recreation, context loss, and backend-independent validation.

OpenGL ES 3.x is required by the Android smoke application. The Android NDK documentation recommends checking the runtime GL version/capabilities rather than inferring support from the device alone. citeturn3search3

## Threading

Android renderer lifecycle changes must account for SurfaceView callbacks, pause/resume, native thread startup/shutdown, JNI object lifetime, ANativeWindow reference counting, and concurrent restart requests.

Do not introduce polling loops that busy-spin when a surface or resource is unavailable.

## Pull requests

A pull request should describe:

1. the problem;
2. implementation and invariants;
3. tests performed;
4. Linux/Termux/Android/GLES impact;
5. API/ABI/build compatibility impact;
6. known limitations and device-validation status.

Keep related changes in one coherent PR rather than creating many tiny PRs.

Before merge, wait for the complete required CI set after the final commit.

## Safe GitHub maintenance workflow

Repository maintenance should be performed as a checkpointed sequence:

1. read the current default-branch SHA;
2. create or rebase a clean feature branch from that SHA;
3. make one logical file mutation at a time;
4. record the resulting commit SHA;
5. inspect the branch/PR state before retrying any interrupted mutation;
6. avoid concurrent GitHub mutations and repeated workflow-log polling;
7. create the PR only after the branch diff is coherent;
8. wait for the final CI checks;
9. merge only after checks pass;
10. re-read main and verify the merge SHA.

This prevents an interrupted API call from being mistaken for a failed commit and avoids unnecessary GitHub API pressure.

## Security

Do not disclose vulnerabilities in public issues. Follow SECURITY.md.

## License

By contributing, you agree that your contribution is provided under the repository's MIT License, subject to applicable law.
