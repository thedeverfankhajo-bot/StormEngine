# Termux development

StormEngine supports Termux as a native C++ development environment for the portable engine core.

## Prerequisites

    pkg update
    pkg install clang cmake ninja make git

Ninja is preferred. The helper falls back to Make when Ninja is unavailable.

## Build and test

From the repository root:

    ./scripts/termux-build.sh

Useful low-resource configuration:

    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh

Release validation:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh

Skip the example executable when desired:

    STORM_RUN_SANDBOX=OFF ./scripts/termux-build.sh

The script configures with strict warnings, builds the portable core, runs the complete CTest suite when enabled, and optionally runs the sandbox.

## CMake presets

With CMake 3.20+:

    cmake --list-presets
    cmake --preset termux-debug
    cmake --build --preset termux-debug
    ctest --preset termux-debug

## OpenGL ES boundary

The native Termux build deliberately sets STORM_BUILD_GLES=OFF.

The GLES backend depends on the Android NDK/EGL/OpenGL ES toolchain and Android platform libraries. A Termux installation is not an Android cross-compilation toolchain, so enabling GLES in a generic Termux host build is rejected by CMake instead of failing later with confusing missing-header or linker errors.

## Android APKs

Android APK construction is a separate SDK/NDK/Gradle workflow. Use the Android project and CI for this validation.

## Device testing

Physical Android hardware is required for meaningful validation of EGL lifecycle, GPU driver behavior, shader compilation, texture formats, buffer lifetime, pause/resume, rotation, surface recreation, and context-loss recovery.

Record the exact device, Android API level, ABI, SoC/GPU, OpenGL ES version, renderer string, commit, and observed results.

## Resource-conscious workflow

On phones with limited RAM/storage:

- use STORM_BUILD_JOBS=1 or 2;
- prefer Debug for rapid regression testing;
- use Release for final performance-oriented validation;
- avoid parallel Git operations and unnecessary rebuild directories;
- remove obsolete build directories when storage becomes constrained.

Termux is a development environment, not an APK packaging replacement.
