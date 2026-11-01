# ADR-0001: Mobile engine boundaries and technology decisions

- Date: 2026-11-01
- Status: Accepted for the engine/platform boundaries; provisional for editor and rendering technology choices
- Scope: Foundation and the next implementation phases

## Context

StormEngine is a mobile-only product targeting Android and iOS. Desktop operating systems may be used for engineering, CI, cross-compilation, and host-side tests, but they are not runtime or editor product targets.

The repository currently contains a C++20/CMake foundation, a small core lifecycle model, platform-neutral lifecycle and surface contracts, host tests, and a host CI workflow. It does not yet contain Android or iOS native adapters, a game loop, a renderer, physics, an editor, or export tooling.

Technology choices must preserve a testable platform-neutral core while respecting the distinct Android and Apple graphics, application-lifecycle, build, and distribution environments.

## Decisions

### 1. Keep the engine core platform-neutral C++20

**Accepted.**

- Keep Foundation free of Android, Apple, Flutter, and graphics-API headers.
- Keep Core dependent on Foundation, not on Editor or concrete rendering backends.
- Keep platform adapters outside the portable core and communicate through narrow, typed contracts.
- Use CMake for shared native engine targets and explicit platform-specific toolchains for Android and iOS.
- Host builds are engineering/test configurations only. They must not create a supported desktop runtime or editor.

This continues the dependency direction already used by the current CMake targets.

### 2. Preserve distinct spatial models for 1D, 2D, and 3D

**Accepted.**

- 1D gameplay uses scalar coordinates, scalar velocity/acceleration, and one-axis collision/queries.
- 2D gameplay uses planar vectors and 2D-specific collision and rendering semantics.
- 3D gameplay uses 3D transforms, geometry, collision, and rendering.
- A 1D world may use a 2D presentation layer, but presentation must not change the gameplay model or physics dimensionality.

Shared low-level math may be reused where its semantics are genuinely common; the public dimensional APIs must remain explicit.

### 3. Use a narrow C ABI at a Dart/native boundary

**Provisional; apply only if Flutter is selected for the mobile editor.**

If the editor uses Flutter, expose engine operations through a small, versioned C ABI facade and call it with Dart FFI. Do not expose C++ classes, STL containers, exceptions, templates, or compiler-specific ABI details across the FFI boundary. Use opaque handles, fixed-width values, explicit ownership rules, and typed error codes.

Use platform channels or native plugin code for operating-system services that require Android or Apple APIs. Do not route high-frequency engine data through message channels when a carefully designed FFI interface is appropriate.

Flutter's current documentation recommends the `package_ffi` template for native bindings in Flutter/Dart projects since Flutter 3.38. This is a technology candidate, not a commitment to use Flutter before a mobile editor prototype is evaluated.

### 4. Keep rendering backend-neutral; evaluate native mobile backends

**Accepted boundary; backend choices remain provisional.**

- Gameplay and scene APIs must not expose Vulkan or Metal handles.
- The renderer-facing contract must define resource ownership, frame/surface lifetime, synchronization, capability discovery, and device-loss behavior without pretending the APIs are identical.
- Metal is the native graphics API candidate for iOS.
- Vulkan is the leading low-level graphics candidate for Android, with runtime capability and feature-profile checks.
- Android Vulkan support must not be inferred solely from the OS version. The driver and physical device capabilities must be queried.
- An OpenGL ES backend is a compatibility candidate only if the supported-device baseline and device tests justify maintaining it. It is not to be added merely because it is familiar.
- No graphics backend is considered implemented until it builds for its target, renders a known test scene, and has evidence from physical devices.

Android's current guidance describes Vulkan as its primary low-level graphics API, while noting that Vulkan versions depend on both Android version and driver support. Its recommended minimum profile for Vulkan support uses Android 10/API 29 or later, Vulkan 1.1 or later, and compatible Android Baseline Profile capabilities. Older-device support may require a separately validated fallback.

### 5. Respect native build and distribution constraints

**Accepted.**

- Android native builds use the Android SDK/NDK and Gradle integration.
- iOS builds and archives use Xcode on macOS; a non-macOS host must not be reported as having produced an iOS archive.
- Signing identities, provisioning profiles, keystores, and distribution credentials remain outside the repository.
- Cross-platform source code does not imply cross-platform build capability from every host operating system.

If Flutter is selected, its documented toolchain constraints also apply: Flutter Android builds can be prepared on supported engineering hosts, while iOS targeting requires macOS.

### 6. Defer third-party engine subsystems until a measured selection

**Accepted.**

No physics, scripting, audio, model-import, or image-decoding dependency is selected by this ADR. Each candidate must be evaluated for:

1. License compatibility and redistribution obligations.
2. Android and iOS build support.
3. Maintenance and security posture.
4. Runtime and memory behavior on representative mobile devices.
5. API fit, testability, and upgrade cost.
6. Actual binary-size impact.

Do not implement placeholder wrappers that imply a dependency has already been integrated.

## Consequences

- The current Foundation and Platform contracts can remain host-testable without pulling in mobile SDKs.
- Android and iOS adapters can be developed and tested independently.
- The editor framework, minimum device profiles, rendering backends, and physics library remain open decisions pending small, measurable prototypes.
- Public APIs must not expose native handles or backend-specific object types unless isolated behind an explicit platform/backend adapter.
- Compatibility documentation must distinguish source implementation, host test evidence, target builds, simulator tests, and physical-device verification.

## Required evidence before final technology selection

| Decision area | Required evidence |
|---|---|
| Flutter editor | Android and iOS interaction prototype, viewport/input latency, memory footprint, project-file access, native FFI lifecycle and crash behavior |
| Android renderer | Vulkan capability distribution against the intended device baseline, driver validation, rendering tests, thermal and battery measurements |
| iOS renderer | Metal device builds, surface recreation, GPU resource lifetime, frame pacing, thermal and battery measurements |
| Physics | Representative 1D/2D/3D benchmarks, determinism requirements, collision correctness, license review, mobile memory profile |
| Scripting | Startup/runtime cost, sandbox threat model, memory/resource limits, debugging experience, app-store policy review |
| Export | Reproducible Android package builds and an Xcode archive/signing pipeline using protected credentials |

## References

- Android Developers — [Use Vulkan for graphics](https://developer.android.com/games/develop/vulkan/overview)
- Android Developers — [Native and proprietary engines: Vulkan support](https://developer.android.com/games/develop/vulkan/native-engine-support)
- Apple Developer — [Metal](https://developer.apple.com/metal/)
- Flutter documentation — [Bind to native code using FFI](https://docs.flutter.dev/platform-integration/bind-native-code)
- Flutter documentation — [Platform integration](https://docs.flutter.dev/platform-integration)
- Apple Developer — [Distributing your app for beta testing and releases](https://developer.apple.com/documentation/xcode/distributing-your-app-for-beta-testing-and-releases)

## Validation status

This ADR records architecture boundaries and a shortlist of technology candidates. It does not mean that Flutter, Vulkan, Metal, a physics library, or any export workflow has been integrated or validated in StormEngine.
