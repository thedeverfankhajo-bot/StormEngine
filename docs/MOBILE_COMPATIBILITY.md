# Mobile compatibility

StormEngine currently targets Android through the NDK, CMake, EGL, and OpenGL ES. iOS/Metal is not implemented.

## Android build targets

| ABI | Architecture | Intended use |
| --- | --- | --- |
| arm64-v8a | AArch64 | Primary physical-device target |
| armeabi-v7a | 32-bit ARM | Compatibility target |
| x86_64 | x86-64 | Emulator/build target |
| x86 | 32-bit x86 | Emulator/build target |

These are build targets, not device certification.

## Representative device families

Physical validation is organized into five families:
1. Samsung Galaxy
2. Xiaomi / Redmi / POCO
3. Google Pixel
4. OnePlus
5. Motorola

A family entry is only a test category. Record the exact model, Android API, SoC/GPU, ABI, OpenGL ES version, renderer string, engine commit, lifecycle results, and observed errors.

## Required lifecycle checks

For every exact device:
1. Launch the smoke application.
2. Verify EGL initialization and GLES 3.x shader compilation.
3. Verify repeated 3D frames without visible corruption.
4. Background/foreground the app and verify renderer restart.
5. Destroy/recreate the surface and verify renderer restart.
6. Test rotation/configuration when enabled by the activity.
7. Exercise context-loss recovery when the device/test harness can trigger it.
8. Verify stale resource handles are rejected after destruction.
9. Record crashes, GL errors, frame stalls, and visual corruption.

A successful APK build is not physical-device certification.

## Termux

Termux is a native development environment for the portable C++ core.

    pkg update
    pkg install clang cmake ninja make git
    ./scripts/termux-build.sh

For constrained phones:
    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

Optional EGL/GLES host build:
    STORM_BUILD_GLES=ON ./scripts/termux-build.sh

This option requires compatible development libraries and is separate from the Android NDK build.

## Android versus Termux

| Capability | Termux | Android SDK/NDK/Gradle |
| --- | --- | --- |
| C++20 core | Yes | Yes |
| CTest | Yes | Android APK build disables host tests |
| JNI library | No | Yes |
| APK packaging | No | Yes |
| NDK cross compilation | No | Yes |
| Physical GPU validation | No | Yes |

## Evidence rule

Never mark a device family as supported from ABI/build success alone. Keep exact-device evidence with commit and renderer information.
