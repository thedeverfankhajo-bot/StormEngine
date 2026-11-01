# StormEngine

StormEngine is an early-stage, mobile-only game-engine project targeting **Android and iOS**. Its intended game models are 1D, 2D, and 3D. The editor is also intended to run only on mobile devices.

> **Current status: C++ foundation with an Android NativeActivity lifecycle bridge.** The platform-neutral C++ foundation cross-compiles for Android arm64 and iOS device arm64. An Android NativeActivity entry point now maps lifecycle and native-window metadata into the platform contracts and is compiled/linked as a shared library in CI. This remains compile-only evidence: the repository does not yet contain an installable Android/iOS application, game loop, renderer, physics engine, editor, asset importer, or Android/iOS export pipeline. Do not mistake the native shared library for a usable game engine or a mobile app.

## Product constraints

- Runtime targets: Android and iOS only.
- Editor targets: Android and iOS only.
- Host operating systems may be used for engineering, CI, cross-compilation, and host-side tests; they are not product targets.
- All repository development is performed on main.
- Platform and graphics capabilities are reported only after implementation and validation on real target devices.

See [Mobile-only policy](docs/MOBILE-ONLY-POLICY.md), [Architecture](docs/ARCHITECTURE.md), [Engine Lifecycle](docs/ENGINE-LIFECYCLE.md), [Compatibility](docs/COMPATIBILITY.md), [Testing](docs/TESTING.md), [Android NativeActivity bridge](docs/ANDROID-NATIVE-ACTIVITY.md), [API versioning](docs/API-VERSIONING.md), and [Roadmap](docs/ROADMAP.md).

## Current foundation

- C++20 and CMake project configuration.
- Typed Error and Result<T> value types.
- A platform-neutral, injectable logging interface.
- An atomic byte-budget accounting utility. It accounts for reservations; it does not allocate memory.
- A minimal engine initialization/shutdown state model. It does not start a game loop or initialize an operating-system runtime.
- A platform-neutral lifecycle state machine with validated transitions.
- A bounded, allocation-free, multi-producer/single-consumer lifecycle event queue with explicit full/closed results.
- A platform-neutral surface lifecycle contract with validated dimensions/scale and generation-based stale-callback rejection.
- An Android NativeActivity adapter built with the NDK Native App Glue. It maps Activity lifecycle events and attaches/resizes/detaches surface metadata without retaining an ANativeWindow pointer.
- Host tests for Foundation, surface lifecycle, and Android lifecycle event normalization.
- CI cross-compilation of the Foundation, Core, and Platform static libraries for Android arm64-v8a and iOS device arm64, plus Android NativeActivity shared-library compilation/linking.

The Android CI job uses ANDROID_PLATFORM=android-21 as its native compilation target. This is not yet a declared product minimum SDK. The iOS CI job uses the iPhoneOS Device SDK and does not create an Xcode application, archive, or IPA.

The Android bridge is only an event/lifecycle adapter. It does not provide frame scheduling, rendering, touch input, sensor access, low-memory policy, Java/Kotlin UI, or a packageable app module. iOS native lifecycle integration is not implemented. Neither target has physical-device runtime validation.

## Build and test the host foundation

Requirements: CMake 3.24 or newer, Ninja for the preset workflow, and a C++20 compiler.

    cmake -S . -B build -DSTORMENGINE_BUILD_TESTS=ON
    cmake --build build --config Debug
    ctest --test-dir build --output-on-failure

Host-side test binaries are engineering tools only; they are not supported StormEngine runtime targets or editor targets.

## Architecture direction

The engine core will remain independent of editor UI and platform rendering APIs. Android and iOS integrations will sit behind explicit platform interfaces. Rendering backends will be selected based on the capabilities of each mobile platform and device; no single graphics API is assumed to be universally available.

## License

No license has been selected or added yet. Until a license is approved and committed, do not assume that this repository grants permission to reuse or redistribute its contents.
