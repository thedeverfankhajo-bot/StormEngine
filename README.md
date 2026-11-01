# StormEngine

StormEngine is an early-stage, mobile-only game-engine project targeting **Android and iOS**. Its intended game models are 1D, 2D, and 3D. The editor is also intended to run only on mobile devices.

> **Current status: C++ foundation, Android/iOS lifecycle adapters, fixed-step Core loop, lifecycle-aware Mobile Runtime frame driver, and a generation-safe Scene/Entity hierarchy, a separate Transform3D math layer, and a bounded versioned Scene archive with explicit component codecs, plus an initial scalar 1D physics world.** The platform-neutral C++ foundation cross-compiles for Android arm64 and iOS device arm64. An Android NativeActivity entry point maps lifecycle and native-window metadata into the platform contracts and is compiled/linked as a shared library in CI. The Core module now provides a frame coordinator that runs input, events, bounded fixed updates, variable update, and render callbacks in a defined order. Core also contains a synchronous, exact-type event dispatcher with move-only RAII subscriptions and explicit handler-error propagation. This remains compile-only evidence for mobile targets: the repository does not yet contain an installable Android/iOS application, an integrated platform game loop, renderer, physics engine, editor, asset importer, or Android/iOS export pipeline. Do not mistake the native shared library or frame coordinator for a usable game engine or a mobile app. The initial 1D physics slice is host-tested/target-built separately from any renderer; it is not yet a complete 1D gameplay runtime.

## Product constraints

- Runtime targets: Android and iOS only.
- Editor targets: Android and iOS only.
- Host operating systems may be used for engineering, CI, cross-compilation, and host-side tests; they are not product targets.
- All repository development is performed on main.
- Platform and graphics capabilities are reported only after implementation and validation on real target devices.

See [Mobile-only policy](docs/MOBILE-ONLY-POLICY.md), [Architecture](docs/ARCHITECTURE.md), [Engine Lifecycle](docs/ENGINE-LIFECYCLE.md), [Game Loop](docs/GAME-LOOP.md), [Mobile Runtime Frame Driver](docs/MOBILE-RUNTIME.md), [Event System](docs/EVENT-SYSTEM.md), [Scene System](docs/SCENE-SYSTEM.md), [Scene Serialization](docs/SCENE-SERIALIZATION.md), [1D Physics](docs/1D-GAME-DEVELOPMENT.md), [Transform System](docs/TRANSFORM-SYSTEM.md), [Compatibility](docs/COMPATIBILITY.md), [Testing](docs/TESTING.md), [Android NativeActivity bridge](docs/ANDROID-NATIVE-ACTIVITY.md), [iOS lifecycle adapter](docs/IOS-LIFECYCLE.md), [API versioning](docs/API-VERSIONING.md), and [Roadmap](docs/ROADMAP.md).

## Current foundation

- C++20 and CMake project configuration.
- Typed Error and Result<T> value types.
- A platform-neutral, injectable logging interface.
- An atomic byte-budget accounting utility. It accounts for reservations; it does not allocate memory.
- A minimal engine initialization/shutdown state model. It does not initialize an operating-system runtime.
- A fixed-step scheduler that accumulates elapsed nanoseconds, caps accepted frame deltas and per-frame catch-up work, reports discarded backlog, and provides a render interpolation alpha. It does not own a clock.
- A callback-driven Core game-loop coordinator with deterministic phase order: input, events, fixed updates, variable update, and render. It accepts elapsed time from its caller and remains independent of Android/iOS APIs.
- A separate Runtime MobileFrameDriver that gates Core callbacks on foreground-active lifecycle and an attached surface, detects lifecycle revisions and surface generations, validates monotonic timestamps, and resets simulation timing after suspension or surface recreation.
- A synchronous Core event dispatcher keyed by exact C++ event type, with ordered handlers, move-only RAII subscriptions, mutation-safe dispatch snapshots, and propagated handler errors.
- An initial Core Scene/Entity system with non-reused Scene IDs, generation-checked entity handles, scene-local typed component storage, automatic component cleanup, and a validated parent/child hierarchy. It is not yet an archetype ECS and has no reflection, automatic component discovery, or editor integration.
- A separate Math target with finite-value checks, normalized quaternions, axis-angle construction, and column-major 4x4 matrices. Core TransformSystem composes optional local Transform3D components through the entity hierarchy on demand.
- A separate Serialization target with bounded little-endian binary primitives and a version-1 Scene archive. Component payloads require explicit stable identifiers and schema-versioned codecs; Transform3D has a built-in codec. Serialization is in-memory only and does not perform filesystem I/O.
- An optional `Storm::Physics1D` target with scalar body state, generation-checked world-local handles, Static/Kinematic/Dynamic bodies, semi-implicit Euler gravity/force integration, impulses, 1D interval contacts, triggers, collision masks, restitution response, and transactional bounded steps. It is discrete, single-thread-affine, and uses a bounded O(n²) pair scan; no 2D/3D physics or renderer exists yet.
- Explicit timing reset for lifecycle suspension/resume; the platform runtime must decide when a frame may run.
- A platform-neutral lifecycle state machine with validated transitions.
- A bounded, allocation-free, multi-producer/single-consumer lifecycle event queue with explicit full/closed results.
- A platform-neutral surface lifecycle contract with validated dimensions/scale and generation-based stale-callback rejection.
- An Android NativeActivity adapter built with the NDK Native App Glue. It maps Activity lifecycle events and attaches/resizes/detaches surface metadata without retaining an ANativeWindow pointer.
- A platform-neutral iOS scene-event adapter plus an Objective-C++ StormEngineSceneDelegate available on iOS 13+ that consumes UIKit scene callbacks, owns one lifecycle state machine per scene, and exposes an overridable transition hook to the host runtime. Its source is compiled only for the iOS target.
- Host tests for Foundation, Math, TransformSystem, fixed-step scheduling, frame orchestration, typed event dispatch, Scene/Entity and component storage, surface lifecycle, Android/iOS lifecycle normalization, and Mobile Runtime gating. All 117 named cases across twelve CTest executables pass in GCC, Clang, Apple Clang, and Clang ASan/UBSan host CI (Scene archive validation: [41322123527](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41322123527)).
- CI cross-compilation of Foundation, Math, Core, Serialization, Platform, and Runtime libraries for Android arm64-v8a and iOS device arm64; the built-in Transform3D SceneSerializer path is explicitly instantiated in both mobile builds. Android NativeActivity shared-library compilation/linking is also checked.

The Android CI job uses ANDROID_PLATFORM=android-21 as its native compilation target. This is not yet a declared product minimum SDK. The iOS CI job uses the iPhoneOS Device SDK and does not create an Xcode application, archive, or IPA.

The Android bridge is only an event/lifecycle adapter. It is not yet wired to the Runtime MobileFrameDriver and does not provide frame pacing, rendering, touch input, sensor access, low-memory policy, Java/Kotlin UI, or a packageable app module. MobileFrameDriver accepts monotonic timestamps supplied by a future native frame source; neither Android Choreographer nor iOS CADisplayLink is integrated. The iOS scene delegate is compiled for the iOS target but is not connected to a complete application/runtime consumer. No renderer, 2D/3D physics implementation, complete iOS application target, signed app, APK/AAB/IPA pipeline, or physical-device runtime validation exists. The first 1D physics model is a discrete host-side simulation library and has no rendering/input/device integration.

## Build and test the host foundation

Requirements: CMake 3.24 or newer, Ninja for the preset workflow, and a C++20 compiler.

    cmake -S . -B build -DSTORMENGINE_BUILD_TESTS=ON
    cmake --build build --config Debug
    ctest --test-dir build --output-on-failure

Host-side test binaries are engineering tools only; they are not supported StormEngine runtime targets or editor targets.

## Architecture direction

The engine core will remain independent of editor UI, platform lifecycle headers, and rendering APIs. Android and iOS integrations will sit behind explicit platform interfaces. Rendering backends will be selected based on the capabilities of each mobile platform and device; no single graphics API is assumed to be universally available.

## License

No license has been selected or added yet. Until a license is approved and committed, do not assume that this repository grants permission to reuse or redistribute its contents.
