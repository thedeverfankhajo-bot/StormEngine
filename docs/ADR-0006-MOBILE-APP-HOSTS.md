# ADR-0006: Native Mobile Application Host Targets

- Status: Accepted for the initial host-shell milestone
- Date: 2026-11-01
- Scope: Android and iOS application packaging

## Context

StormEngine already contains an Android NativeActivity lifecycle bridge, an iOS UIKit scene lifecycle delegate, platform-neutral lifecycle contracts, and a Runtime frame driver. It did not contain an Android application module or an iOS application target. Static-library cross-compilation alone could not validate application packaging or entry-point linkage.

The next milestone therefore needs thin native application hosts without pretending that rendering, touch input, or the full game runtime already exist.

## Decision

- Android uses the NDK NativeActivity entry point already present in the repository, packaged by a Gradle application module.
- The initial Android host package targets arm64-v8a, uses minSdk 21 and targetSdk 36, and produces a debug APK. Release signing, AAB generation, and additional ABIs are out of scope.
- iOS uses a CMake/Xcode `MACOSX_BUNDLE` executable that links the existing UIKit scene lifecycle adapter and produces an unsigned device app bundle.
- The initial iOS deployment target is 13.0, matching the existing `UIWindowSceneDelegate` integration.
- The native hosts remain thin. They do not own engine business logic or introduce graphics API dependencies into Core.
- CI validates the produced Android APK and iOS app bundle. This is package/build evidence, not physical-device runtime validation.

## Toolchain selection

The Android host build pins its build-tool versions: Android Gradle Plugin 9.4.0, Gradle 9.6.0, JDK 17, SDK 36, Build Tools 36.0.0, NDK 28.2.13676358, and CMake 3.31.0. These versions are selected for a repeatable CI build and can be revised through a later ADR after compatibility review.

## Consequences

Positive:
- CI exercises the Android application packaging boundary and the iOS app-bundle target rather than only static libraries.
- The project gains concrete native host entry points while preserving the mobile-only product boundary.
- No runtime third-party library dependency is introduced.

Limitations:
- The Android host currently waits in the NativeActivity event pump and does not render.
- The iOS host displays a diagnostic status screen and does not initialize a game runtime.
- Neither host is connected to the Runtime frame driver or touch input.
- Neither platform has been exercised on a physical device.
- The Android artifact is debug-only; the iOS artifact is unsigned.

## Validation criteria

This milestone is build-tested only when the Android APK contains `libstorm_android_native_activity.so`, the iOS `.app` bundle is produced by the iOS Device SDK build, and the corresponding GitHub Actions jobs pass. Physical-device compatibility remains unverified until named devices and OS versions are tested.
