# Compatibility Matrix

Last reviewed: 2026-11-01

Statuses distinguish source implementation, target-architecture compilation, and target-device runtime validation. A successful cross-build does not establish runtime compatibility.

| Capability | Android | iOS | Backend | Implementation status | Test status | Limits / evidence |
|---|---|---|---|---|---|---|
| C++20 Foundation primitives | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Android NDK / Apple Clang + iPhoneOS SDK | Implemented | Host tests and target static-library builds passed in GitHub Actions | Compile-only. Android CI currently passes ANDROID_PLATFORM=android-21; this is not a product minimum SDK. No app package or device execution. |
| Engine initialization/shutdown model | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host lifecycle tests and both target cross-builds passed | State model only; no OS runtime startup, application entry point, or game loop. |
| Fixed-step timing scheduler | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 / Storm::Core | Implemented | Seven dedicated host tests passed under GCC, Clang, Apple Clang, and Clang ASan/UBSan; Core target cross-builds passed in Android NDK and iOS Device SDK CI | Scheduling utility only; not connected to a platform clock, game loop, or physics callbacks. No device timing validation. |
| Normalized lifecycle state machine | Android bridge consumes the normalized contract | Cross-compiled: device arm64; no UIKit adapter | Platform-neutral C++20 | Implemented | Host transition tests and Android/iOS target builds passed | iOS OS lifecycle events are not yet delivered. Android callback runtime has not been exercised on a device. |
| Lifecycle event queue | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host FIFO/capacity/concurrency tests and both target cross-builds passed | Bounded 64-event FIFO queue; native adapters must handle Full explicitly. Android NativeActivity callbacks are serialized on the app thread and do not currently use this queue. |
| Surface lifecycle contract | Connected to Android ANativeWindow attach/resize/detach metadata | Cross-compiled: device arm64; no UIKit surface adapter | Platform-neutral contract + Android NDK | Implemented | Host contract tests and Android/iOS target builds passed | Android bridge stores only generated opaque IDs and surface metadata; it does not retain ANativeWindow pointers, create a renderer, or manage GPU resources. |
| Android NativeActivity lifecycle | NDK arm64-v8a shared library cross-build | N/A | Android NDK Native App Glue | Implemented | Host event-policy tests and Android shared-library compile/link in CI | No Android emulator/physical-device execution, APK/AAB packaging, renderer, input handling, or game loop. See [Android NativeActivity bridge](ANDROID-NATIVE-ACTIVITY.md). |
| iOS native lifecycle | N/A | Planned | UIKit / Objective-C++ | Planned | Not tested | No UIApplication/UIScene callback adapter or iOS application target. |
| Rendering | Planned | Planned | TBD; platform-native backends | Planned | Not tested | No renderer, GPU backend, shader pipeline, or device rendering test exists. |
| 1D / 2D / 3D gameplay systems | Planned | Planned | Engine-level | Planned | Not tested | No spatial simulation, scene system, physics, or gameplay rendering implementation exists. |
| Mobile editor | Planned | Planned | TBD | Planned | Not tested | No Flutter/Dart or native editor application exists. |
| Android packaging | Planned | N/A | Gradle / Android SDK | Planned | Not tested | Native shared-library cross-build only; no APK/AAB pipeline exists. |
| iOS packaging | N/A | Planned | Xcode | Planned | Not tested | Cross-compilation creates static libraries only; no app target, archive, signing, or IPA pipeline exists. |

## Graphics API notes

Vulkan is an Android graphics option but is not present on every eligible device; device capability checks and a documented minimum profile will be required. iOS uses Metal as its native graphics API. Vulkan on iOS would require a translation layer such as MoltenVK and must not be described as native iOS Vulkan support. These are architecture constraints, not implemented backends.

References:

- [Android Developers: Use Vulkan for graphics](https://developer.android.com/games/develop/vulkan/overview)
- [Vulkan Guide: Platform support](https://docs.vulkan.org/guide/latest/platforms.html)
- [Apple Developer: Metal](https://developer.apple.com/documentation/metal)

## Validation rules

- Planned: no implementation exists.
- Experimental: implementation exists but interfaces, behavior, or compatibility are not stable.
- Implemented: source code exists.
- Tested: relevant automated tests passed in the stated environment. Cross-compilation is recorded separately from runtime testing.
- Verified: target runtime behavior was validated on named physical devices and OS versions.
- Unsupported: intentionally not supported by the product.

The project currently has no Android- or iOS-verified runtime features. The latest successful CI run for commit `fd1e5ae223c485ad19a0585e76b083f66acb519c` completed all six jobs: GCC, Clang, Clang ASan/UBSan, Apple Clang, Android arm64-v8a, and iOS device arm64. This is host-test and target-build evidence, not physical-device runtime verification. CI does not produce or run a mobile application.
