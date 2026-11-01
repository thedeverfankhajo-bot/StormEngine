# Compatibility Matrix

Last reviewed: 2026-11-01

Statuses distinguish source implementation from target-platform validation. Host-side success does not establish Android or iOS compatibility.

| Capability | Android | iOS | Backend | Implementation status | Test status | Limits / evidence |
|---|---|---|---|---|---|---|
| C++20 Foundation primitives | Planned | Planned | None | Implemented | Tested on Linux host | GCC 14.2 and Clang 17 builds passed locally; no mobile cross-build yet |
| Engine initialization/shutdown model | Planned | Planned | None | Implemented | Tested on Linux host | Lifecycle model only; no OS integration or game loop |
| Android native lifecycle | Planned | N/A | Android SDK / NDK | Planned | Not tested | Android SDK and NDK were unavailable in the current build environment |
| iOS native lifecycle | N/A | Planned | iOS SDK / Xcode | Planned | Not tested | Xcode was unavailable in the current build environment |
| Rendering | Planned | Planned | TBD; native candidates differ by platform | Planned | Not tested | No backend or device rendering test exists |
| 1D / 2D / 3D gameplay systems | Planned | Planned | Engine-level | Planned | Not tested | No spatial simulation or rendering implementation exists |
| Mobile editor | Planned | Planned | TBD | Planned | Not tested | No Flutter/Dart or native editor application exists |
| Android packaging | Planned | N/A | Gradle / Android SDK | Planned | Not tested | No export pipeline exists |
| iOS packaging | N/A | Planned | Xcode | Planned | Not tested | No export pipeline exists |

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
- Tested: relevant automated tests passed in the stated environment.
- Verified: target builds and runtime behavior were validated on named physical devices and OS versions.
- Unsupported: intentionally not supported by the product.

The project currently has no Android- or iOS-verified features.
