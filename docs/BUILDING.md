# Building StormEngine

## Requirements

- CMake 3.25 or newer.
- A C++20-capable compiler: GCC, Clang/AppleClang or MSVC.
- Git, if cloning the repository.
- Ninja is optional; the examples below let CMake choose the default generator.

## Linux, macOS and Windows

From the repository root:

1. Configure: `cmake -S . -B build -DSTORM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug`
2. Build: `cmake --build build --config Debug --parallel 2`
3. Test: `ctest --test-dir build -C Debug --output-on-failure`

On single-configuration generators, CMAKE_BUILD_TYPE controls the build type. Visual Studio and other multi-configuration generators use the --config option at build/test time.

A sanitizer build is available with GCC/Clang-family toolchains: configure with `-DSTORM_ENABLE_SANITIZERS=ON`. Sanitizer availability depends on the host runtime and is not enabled by default.

## Android NDK cross-compilation

The core is currently a native library; this does **not** create a complete Android application or APK. Install Android Studio/SDK and an NDK version supported by your Android Gradle Plugin/toolchain, then set ANDROID_NDK_HOME to that NDK directory.

On a POSIX shell:

1. Configure for a 64-bit ARM device: `cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_STL=c++_shared -DSTORM_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release`
2. Build: `cmake --build build-android --config Release --parallel 2`

Use Android Studio/Gradle with externalNativeBuild when integrating this library into an Android application. Do not hard-code compiler options in Gradle when those options belong to CMake targets. Verify the chosen NDK, Android Gradle Plugin, minimum API level and ABI against the official Android documentation for the application being built.

## Current scope and limitations

The first milestone verifies only the core library and unit tests. There is no Android activity, APK, window/surface abstraction, renderer, editor or asset pipeline yet. A successful cross-compile alone is not evidence that the engine is ready for Android gameplay.

## Troubleshooting

- Confirm CMake and compiler versions before debugging source code.
- Delete the build directory when intentionally changing generators or toolchains.
- Do not commit generated build directories, SDK paths, local.properties, binaries, or signing secrets.
- Share the full first compiler error and the configure command when reporting a build issue; later errors may be cascading failures.
