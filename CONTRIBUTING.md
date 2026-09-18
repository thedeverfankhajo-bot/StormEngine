# Contributing to StormEngine

## Scope

StormEngine is an active C++20 engine project. Keep changes focused, testable, portable, and explicit about ownership and lifetime.

## Requirements

- C++20 compiler.
- CMake 3.23+ for repository presets; direct builds can use CMake 3.20+.
- Git.
- Android SDK/NDK and Gradle for Android work.
- Termux users should use scripts/termux-build.sh for native validation.

## Termux validation

Install clang, cmake, ninja (or make), and git, then run:

    ./scripts/termux-build.sh

For resource-constrained Android phones:

    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

The script does not download remote tools or replace the Android SDK/NDK workflow.

Install clang, cmake, ninja (or make), and git, then run:

    ./scripts/termux-build.sh

This validates the portable native engine. It does not replace Android APK validation.

## Host validation

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/storm_sandbox

For GLES/Android changes, validate the Android workflow or equivalent Android build. Do not claim device-level behavior unless tested on a device or emulator.

## Code

- Use C++20 and keep public APIs platform-neutral.
- Preserve resource/entity lifetime and stale-handle invariants.
- Validate finite values, bounds, counts, overflow, and untrusted input.
- Prefer RAII and explicit thread shutdown ordering.
- Keep backend-neutral code independent of OpenGL ES types.
- Add deterministic tests for bug fixes and new behavior.
- Do not claim physical-device compatibility without device evidence.

- Use C++20 and keep public APIs portable unless a platform boundary requires otherwise.
- Preserve resource and entity lifetime invariants.
- Validate external input at subsystem boundaries.
- Check finite values, overflow, bounds, invalid handles, and destruction order.
- Avoid hidden ownership and unnecessary global state.
- Keep backend-neutral code independent of OpenGL ES types.
- Add deterministic regression tests for new behavior and bug fixes.

## Rendering

Consider resource lifetime, shader/program lifetime, vertex/index bounds, render-state transitions, texture/sampler binding, Android/EGL lifecycle, and backend-independent behavior.

## Mobile compatibility

The documented Samsung, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola families are representative validation targets only. Record exact device, Android API, ABI, SoC/GPU, renderer string, commit, and lifecycle test results.

## Pull requests

A pull request should:

1. Describe the problem and implementation.
2. Document invariants, ownership, lifetime, and thread-safety impact.
3. Include deterministic regression tests where practical.
4. State Linux/Android/GLES/Termux and ABI impact.
5. List exact validation commands and CI results.
6. Avoid unrelated formatting/refactoring churn.

Do not merge with unexplained failing checks.

A pull request should describe the problem, implementation, invariants, tests, Linux/Android/GLES impact, API/ABI/build compatibility impact, and validation results.

Do not merge with unexplained failing checks.

## Security

Do not disclose vulnerabilities in public issues. Follow SECURITY.md.

## License

By contributing, you agree that your contribution is provided under the repository's MIT License, subject to applicable law.
