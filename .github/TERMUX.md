# Termux development

StormEngine supports Termux as a native C++ development and test environment.

## Required packages

    pkg update
    pkg install clang cmake ninja make git

## Reproducible build

    ./scripts/termux-build.sh

The helper validates build-mode inputs, uses Ninja when available and Make otherwise, bounds parallelism through STORM_BUILD_JOBS, runs CTest, and runs the native sandbox when available.

Examples:

    STORM_BUILD_TYPE=Release ./scripts/termux-build.sh
    STORM_BUILD_JOBS=1 ./scripts/termux-build.sh
    STORM_BUILD_TESTS=OFF ./scripts/termux-build.sh

## CMake presets

Repository presets use schema version 4 and therefore require CMake 3.23+:

    cmake --list-presets
    cmake --preset termux-debug
    cmake --build --preset termux-debug
    ctest --preset termux-debug

The termux-gles-debug preset is optional and requires compatible EGL/GLES development libraries.

## Android boundary

Termux builds the portable native engine. It does not replace Android SDK/NDK/Gradle, JNI packaging, or APK validation.

## Resource-constrained phones

Use STORM_BUILD_JOBS=1 on small-memory phones. Keep build directories on storage with sufficient free space. Prefer Debug for iteration and Release for final smoke validation.

## Security

The helper does not download or execute remote scripts. Do not add network bootstrap logic to it.
