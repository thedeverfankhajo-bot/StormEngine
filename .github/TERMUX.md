# Termux development

StormEngine currently targets Android through the NDK, CMake, EGL, and OpenGL ES. It does not yet provide an iOS/Metal backend.

## Android APK builds

| Target | Architecture | Role |
| --- | --- | --- |
| Android | arm64-v8a | Primary |
| Android | armeabi-v7a | Compatibility build |
| Android | x86_64 | Emulator/build target |
| Android | x86 | Emulator/build target |

These are build targets, not claims that every GPU/device has been physically tested.

## Device testing

The compatibility test matrix uses Samsung Galaxy, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola. A family entry is not certification for every model. Physical GPU/driver validation is required.

## Graphics

The Android renderer requires OpenGL ES 3.x. The manifest declares GLES 3.0 as the minimum feature requirement. Runtime probing must still be used for optional features.

## Device validation checklist

Record Android API level, SoC/GPU, ABI, OpenGL ES version, renderer string, commit, surface recreation, pause/resume, rotation, context-loss recovery, shader compilation, texture upload, buffer lifetime, basic 3D rendering, and visible corruption/frame stability.

A successful APK build is not device certification.

## Termux

Termux is a first-class native C++ development environment. It is not a replacement for the Android Gradle/NDK cross-compilation toolchain.

Install:

    pkg update
    pkg install clang cmake ninja make git

Build:

    bash ./scripts/termux-build.sh

Release:

    STORM_BUILD_TYPE=Release bash ./scripts/termux-build.sh

Optional GLES host build:

    STORM_BUILD_GLES=ON bash ./scripts/termux-build.sh

Only enable host GLES when the required EGL/GLES development libraries are available.

Android APK builds still require an Android SDK/NDK and Gradle environment.
