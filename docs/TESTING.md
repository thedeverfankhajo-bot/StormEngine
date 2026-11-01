# Testing and CI

## Current automated coverage

The existing host test suite contains 46 named test cases across six CTest executables. All six executables passed in the latest successful Foundation CI run for commit `56f0607856d548a00c06a1b6741f2d178a2ddf0d`. The current change adds a seventh CTest executable with seven iOS lifecycle-policy cases; that new suite's CI result is pending. The previous six-job result remains the evidence for the existing six executables:

- stormengine_foundation_tests (11 cases):
  - Success and failure paths for typed Result<T> values.
  - Memory-budget capacity, over-reservation, release validation, and concurrent reservations.
  - Injectable logging and structured record fields.
  - Engine initialization and shutdown lifecycle.
  - Platform lifecycle transitions, invalid transitions, and background launch.
  - Lifecycle event queue FIFO ordering, capacity/close behavior, and concurrent multi-producer delivery.
- stormengine_platform_surface_tests (4 cases):
  - Surface attach validation and monotonically increasing generations.
  - Resize validation and generation preservation.
  - Detach/recreation behavior and stale-callback rejection.
  - Invalid resize/detach operations when no surface is attached.
- stormengine_android_lifecycle_tests (5 cases):
  - START/RESUME/FOCUS/PAUSE/STOP event normalization.
  - STOP from an active state with an explicit inactive transition.
  - Focus recovery when a separate RESUME event is absent.
  - Idempotent duplicate lifecycle events.
  - Terminal destroy behavior and invalid event rejection.
- stormengine_ios_lifecycle_tests (7 cases; current CI pending):
  - Foreground/background callback sequence.
  - Active callback recovery from background.
  - Background launch and repeated background delivery.
  - Idempotent scene callbacks.
  - Independent lifecycle state for two scenes.
  - Terminal app termination.
  - Unknown scene/application event rejection.
- stormengine_fixed_step_scheduler_tests (7 cases):
  - Invalid step, frame-delta limit, and catch-up budget configuration.
  - Partial frame accumulation and fixed-step emission.
  - Frame-delta clamping.
  - Catch-up budget enforcement while preserving the fractional remainder.
  - Negative-delta rejection without mutating scheduler state.
  - Explicit accumulator reset.
  - Duration-overflow reporting without mutating scheduler state.
- stormengine_game_loop_tests (9 cases):
  - Configuration validation.
  - Deterministic input/event/fixed-update/variable-update/render ordering.
  - Zero-duration frame behavior.
  - Frame-delta clamping and discarded-backlog reporting.
  - Fixed-update and variable-update failure propagation.
  - Early termination on input failure.
  - Negative-delta rejection before callbacks.
  - Timing reset across suspension.
- stormengine_event_dispatcher_tests (10 cases):
  - Subscription-order dispatch and exact event-type matching.
  - Successful dispatch with no listeners.
  - RAII disconnection and move-only subscription behavior.
  - Immediate skip after disconnecting a pending listener during dispatch.
  - New listeners taking effect on the next dispatch.
  - Clearing all listeners during an active dispatch.
  - Handler error propagation and early termination.
  - Rejection of an empty std::function handler.
  - Safe subscription destruction after the dispatcher is gone.
  - Idempotent explicit disconnection.

These are host-side tests. The Android and iOS lifecycle cases test platform-neutral event policies; they do not invoke Android framework or UIKit callbacks. The GameLoop tests do not exercise an Android/iOS clock, native lifecycle gate, GPU, or physical device.

## Local host workflow

With CMake 3.24+, Ninja, and a C++20 compiler installed:

    cmake --preset host-debug
    cmake --build --preset host-debug --parallel
    ctest --preset host-debug

A direct CMake configure/build/test flow is also supported and documented in the repository README.

## GitHub Actions

The Foundation workflow runs on pushes to main and on manual dispatch. The successful target-validation workflow run [41303432277](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41303432277) for commit `56f0607856d548a00c06a1b6741f2d178a2ddf0d` completed all six jobs successfully:

- Linux with GCC: configure, build, and run host CTest.
- Linux with Clang: configure, build, and run host CTest.
- Linux with Clang AddressSanitizer and UndefinedBehaviorSanitizer: build and run host CTest with leak detection enabled.
- Android NDK: cross-compile Foundation, Core, and Platform for arm64-v8a, then compile and link the NativeActivity shared library against the NDK-provided Native App Glue, Android, and Log libraries. The CI configuration uses ANDROID_PLATFORM=android-21 for this compile job.
- iOS Device SDK: cross-compile Foundation, Core, and Platform for arm64 using the iPhoneOS SDK and Xcode.
- macOS with Apple Clang: configure, build, and run host CTest.

The Android and iOS jobs disable host test executables because those binaries cannot be run as ordinary host programs from the target toolchains. They validate compilation and linking only; they do not build APK/AAB/IPA packages, invoke Android callbacks, deploy to devices, or run on physical hardware. The Android API 21 value is a CI compilation target, not a declared product minimum SDK.

The iOS job uses CMake's Xcode generator and the iPhoneOS Device SDK. A successful static-library build is not an iOS application archive or signed IPA.

The sanitizer job runs the same host tests with leak detection enabled and UndefinedBehaviorSanitizer configured to halt on the first reported issue. Sanitizer coverage increases confidence in tested code paths but is not a substitute for a security audit, fuzzing, or device testing.

## Validation boundaries

- A green host CI run proves only that the tested Foundation/Core configuration built and its host tests passed on those runners.
- Android/iOS cross-compilation proves that the current C++ targets compile for the selected target architecture and SDK/toolchain. The Android shared-library link also checks the NativeActivity bridge's NDK symbols. Neither check proves OS callback behavior, app packaging, installability, runtime behavior, graphics availability, or device compatibility.
- AddressSanitizer can detect classes of memory errors on exercised paths; UndefinedBehaviorSanitizer can detect supported forms of undefined behavior on exercised paths. Neither proves the absence of all memory-safety or correctness defects.
- The current workflow does not run ThreadSanitizer, static analysis, dependency vulnerability scanning, fuzz tests, Android instrumentation tests, or iOS simulator/device tests.
- Android/iOS runtime compatibility requires installable native application targets, platform lifecycle integration, target-specific tests, and real-device evidence.

See [Android NativeActivity Lifecycle Bridge](ANDROID-NATIVE-ACTIVITY.md), [Core Game Loop](GAME-LOOP.md), and [Core Event System](EVENT-SYSTEM.md) for the exact implementation scope and limitations.
