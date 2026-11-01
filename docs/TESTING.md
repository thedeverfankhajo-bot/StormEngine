# Testing and CI

## Current automated coverage

The current test executable covers:

- Success and failure paths for typed Result values.
- Memory budget capacity, over-reservation, release validation, and concurrent reservations.
- Injectable logging and structured record fields.
- Engine initialization validation and lifecycle transitions.

These are host-side Foundation tests. They do not establish Android or iOS runtime compatibility.

## Local workflow

With CMake 3.24+, Ninja, and a C++20 compiler installed:

```sh
cmake --preset host-debug
cmake --build --preset host-debug --parallel
ctest --preset host-debug
```

A direct CMake configure/build/test flow is also supported and documented in the repository README.

## GitHub Actions

The Foundation workflow runs on pushes to main and on manual dispatch. It builds and tests:

- Linux with GCC.
- Linux with Clang.
- macOS with the runner's Apple Clang toolchain.

The workflow is intentionally scoped to the currently implemented host-testable Foundation. Android NDK builds, iOS/Xcode builds, device deployment, and physical-device validation are not included because the native platform layer has not been implemented yet.

## Validation boundaries

- A green host CI run proves only that the tested Foundation configuration built and its host tests passed on those runners.
- Sanitizer runs performed locally are not equivalent to a full security audit.
- Android/iOS compatibility requires platform-specific CI and real-device evidence in later phases.
