# Compatibility Matrix

Last reviewed: 2026-11-01

Statuses distinguish source implementation, target-architecture compilation, and target-device runtime validation. A successful cross-build does not establish runtime compatibility.

| Capability | Android | iOS | Backend | Implementation status | Test status | Limits / evidence |
|---|---|---|---|---|---|---|
| C++20 Foundation primitives | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Android NDK / Apple Clang + iPhoneOS SDK | Implemented | Host tests and target static-library builds passed in GitHub Actions | Compile-only. Android CI currently passes `ANDROID_PLATFORM=android-21`; this is not a product minimum SDK. No app package or device execution. |
| Engine initialization/shutdown model | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host lifecycle tests and both target cross-builds passed | State model only; no OS runtime startup, application entry point, or game loop. |
| Normalized lifecycle state machine | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host tests and both target cross-builds passed | Native Android/iOS callback adapters are not implemented; no OS lifecycle events are delivered yet. |
| Lifecycle event queue | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host FIFO/capacity/concurrency tests and both target cross-builds passed | Bounded 64-event FIFO queue; native adapters must handle Full explicitly. No target-device concurrency validation. |
| Surface lifecycle contract | Cross-compiled: arm64-v8a | Cross-compiled: device arm64 | Platform-neutral C++20 | Implemented | Host lifecycle tests and both target cross-builds passed | No native handle ownership, GPU synchronization, swapchain/drawable, or OS surface adapter. |
| Android native lifecycle | Planned | N/A | Android SDK / NDK | Planned | Not tested | No Activity/NativeActivity/JNI lifecycle adapter or Android application module. |
| iOS native lifecycle | N/A | Planned | UIKit / Objective-C++ | Planned | Not tested | No UIApplication/UIScene callback adapter or iOS application target. |
| Rendering | Planned | Planned | TBD; platform-native backends | Planned | Not tested | No renderer, GPU backend, shader pipeline, or device rendering test exists. |
| 1D / 2D / 3D gameplay systems | Planned | Planned | Engine-level | Planned | Not tested | No spatial simulation, scene system, physics, or gameplay rendering implementation exists. |
| Mobile editor | Planned | Planned | TBD | Planned | Not tested | No Flutter/Dart or native editor application exists. |
| Android packaging | Planned | N/A | Gradle / Android SDK | Planned | Not tested | Cross-compilation creates static libraries only; no APK/AAB pipeline exists. |
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

The project currently has no Android- or iOS-verified runtime features. The latest successful CI run built the static Foundation, Core, and Platform libraries for Android arm64-v8a and iOS device arm64; it did not produce or run a mobile application.
