# Termux development

StormEngine supports Termux as a native C++ development environment for the portable engine core.

## Prerequisites

    pkg update
    pkg install clang cmake ninja make git

CMake 3.20+ is sufficient for the native helper. CMake 3.23+ is required when using the shared preset schema version 4. Ninja is preferred; the helper can use Make.

## Build and test

From the repository root:

    ./scripts/termux-build.sh

Low-memory configuration:

    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

Release:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh

Skip the sandbox:

    STORM_RUN_SANDBOX=OFF ./scripts/termux-build.sh

The helper validates the compiler, CMake version, generator, boolean options, and parallelism before configuring. It enables strict warnings, builds the portable core, runs CTest when requested, and runs the sandbox unless disabled.

## CMake presets

    cmake --list-presets
    cmake --preset termux-debug
    cmake --build --preset termux-debug
    ctest --preset termux-debug

## OpenGL ES boundary

The generic Termux native build deliberately defaults to STORM_BUILD_GLES=OFF.

The GLES backend depends on the Android NDK/EGL/OpenGL ES platform libraries. Termux is a native Android user-space environment, but a Termux clang build is not automatically an Android NDK cross-compilation environment. APK/GLES validation therefore remains a separate Android toolchain step.

## Android APKs

Android APK construction uses the Android SDK/NDK/Gradle project under android/. CI builds the configured four ABIs and runs Android lint.

## Device testing

Physical Android hardware is required for meaningful validation of EGL lifecycle, GPU driver behavior, shader compilation, texture/buffer lifetime, pause/resume, rotation, surface recreation, and context-loss recovery.

The representative matrix covers Samsung Galaxy, Xiaomi/Redmi/POCO, Google Pixel, OnePlus, and Motorola. Record the exact model, API, ABI, SoC/GPU, GLES version, vendor/renderer, commit, and result.

## Resource-conscious workflow

On phones with limited RAM/storage:

- use STORM_BUILD_JOBS=1 or 2;
- prefer Debug for rapid regression testing;
- use Release for final validation;
- keep one active build directory per configuration;
- remove obsolete build directories when storage becomes constrained.

Do not add remote install/download steps to the helper. Termux is a development environment, not an APK-signing or packaging replacement.
