# Termux development

StormEngine supports Termux as a native C++ development environment.

## Prerequisites
    pkg update
    pkg install clang cmake ninja make git

## Build and test
From the repository root:
    ./scripts/termux-build.sh

The script uses the Termux compiler and build tools and runs the complete CTest suite when tests are enabled.

## Android APKs
Android APK construction is separate from the native Termux build. It requires Android SDK/NDK and Gradle.

## GLES
STORM_BUILD_GLES=ON is opt-in. A Termux host may not expose the same EGL/GLES development libraries as the Android NDK.

## Device testing
Use physical Android hardware for EGL lifecycle, GPU driver, shader compiler, texture format, pause/resume, rotation, and surface recreation testing.