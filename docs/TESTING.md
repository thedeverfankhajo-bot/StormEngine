# Testing and CI

## Current automated coverage

The host test suite currently contains 15 named test cases across two CTest executables:

- `stormengine_foundation_tests` (11 cases):
  - Success and failure paths for typed `Result<T>` values.
  - Memory-budget capacity, over-reservation, release validation, and concurrent reservations.
  - Injectable logging and structured record fields.
  - Engine initialization and shutdown lifecycle.
  - Platform lifecycle transitions, invalid transitions, and background launch.
  - Lifecycle event queue FIFO ordering, capacity/close behavior, and concurrent multi-producer delivery.
- `stormengine_platform_surface_tests` (4 cases):
  - Surface attach validation and monotonically increasing generations.
  - Resize validation and generation preservation.
  - Detach/recreation behavior and stale-callback rejection.
  - Invalid resize/detach operations when no surface is attached.

These are host-side tests. They do not establish Android or iOS runtime compatibility.

## Local workflow

With CMake 3.24+, Ninja, and a C++20 compiler installed:

```sh
cmake --preset host-debug
cmake --build --preset host-debug --parallel
ctest --preset host-debug
```

A direct CMake configure/build/test flow is also supported and documented in the repository README.

## GitHub Actions

The Foundation workflow runs on pushes to `main` and on manual dispatch. It has four validation jobs:

- Linux with GCC.
- Linux with Clang.
- Linux with Clang AddressSanitizer and UndefinedBehaviorSanitizer enabled.
- macOS with the runner's Apple Clang toolchain.

The sanitizer job runs the same host tests with leak detection enabled and UndefinedBehaviorSanitizer configured to halt on the first reported issue. Sanitizer coverage increases confidence in tested code paths but is not a substitute for a security audit, fuzzing, or device testing.

The workflow is intentionally scoped to the currently implemented host-testable Foundation. Android NDK builds, iOS/Xcode builds, device deployment, and physical-device validation are not included because native platform adapters have not been implemented yet.

## Validation boundaries

- A green host CI run proves only that the tested Foundation configuration built and its host tests passed on those runners.
- AddressSanitizer can detect classes of memory errors on exercised paths; UndefinedBehaviorSanitizer can detect supported forms of undefined behavior on exercised paths. Neither proves the absence of all memory-safety or correctness defects.
- The current workflow does not run ThreadSanitizer, static analysis, dependency vulnerability scanning, or fuzz tests.
- Android/iOS compatibility requires platform-specific CI, native build validation, and real-device evidence in later phases.
