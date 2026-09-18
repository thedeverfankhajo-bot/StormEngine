# Mobile compatibility

StormEngine currently targets Android through the NDK, CMake, EGL, and OpenGL ES. It does not yet provide an iOS/Metal backend.

## Android build targets

| Target | Architecture | Role |
| --- | --- | --- |
| Android | arm64-v8a | Primary modern-device target |
| Android | armeabi-v7a | Compatibility target |
| Android | x86_64 | Emulator/legacy build target |
| Android | x86 | Emulator/legacy build target |

These are build targets, not claims that every GPU/device has been physically tested.

## Representative device families

The validation matrix is intentionally distributed across five families:

1. Samsung Galaxy
2. Xiaomi / Redmi / POCO
3. Google Pixel
4. OnePlus
5. Motorola

A family entry is not certification for every model.

## Required physical-device record

For every device validation run, record:

- exact model;
- Android version/API level;
- ABI;
- SoC/GPU;
- OpenGL ES version;
- GL renderer/vendor strings;
- StormEngine commit;
- APK build variant;
- surface creation/destruction;
- pause/resume;
- rotation/configuration change;
- context-loss/recreation behavior;
- shader compilation;
- texture upload;
- buffer creation/update/destruction;
- basic 3D draw;
- visible corruption/errors;
- sustained frame stability.

## Graphics requirements

The Android renderer requires OpenGL ES 3.x. The manifest declares GLES 3.0 as a required feature.

Runtime probing is still required for optional GPU capabilities. Do not infer a feature solely from a device family or GPU name.

## Termux

Termux is a first-class native C++ development environment for the portable core.

Install:

    pkg update
    pkg install clang cmake ninja make git

Build:

    ./scripts/termux-build.sh

Release:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh

The Termux helper explicitly disables the Android GLES backend. Android APK/GLES validation remains an NDK/Gradle/device workflow.

## Compatibility interpretation

An APK that compiles for all configured ABIs demonstrates packaging/build coverage only. It does not demonstrate that every ABI, GPU driver, vendor extension, Android version, or physical device has passed runtime validation.

Failures should be recorded with exact hardware/software information so driver-specific behavior is distinguishable from engine bugs.
