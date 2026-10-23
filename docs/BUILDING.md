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

AddressSanitizer and UndefinedBehaviorSanitizer are supported with GCC/Clang-family toolchains. Configure and run an instrumented test build with:

```sh
cmake -S . -B build-sanitized -DSTORM_BUILD_TESTS=ON -DSTORM_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sanitized --parallel 2
ctest --test-dir build-sanitized --output-on-failure
```

Both the core library and test translation unit are instrumented. The repository CI runs this additional job on Ubuntu; the sanitizer option remains off by default and is not supported by this configuration on MSVC.

## Install and consume the core package

Install the native library and its CMake package files from an existing build:

```sh
cmake --install build --prefix stage
```

A downstream CMake project can then locate the installed package and link the exported target:

```cmake
find_package(StormEngine 0.1 CONFIG REQUIRED)
target_link_libraries(my_game PRIVATE Storm::Core)
```

Set `CMAKE_PREFIX_PATH` to the installation prefix when configuring the downstream project, or use `StormEngine_DIR` to point directly at the directory containing `StormEngineConfig.cmake`. The package is designed to remain usable after its installation directory is moved. The normal CTest suite includes an isolated consumer build that checks package discovery, linking and execution against a relocated install.

## Secure ID entropy providers

The secure ID generator uses operating-system cryptographic random sources, without third-party runtime packages: [Windows BCryptGenRandom](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom), [Apple SecRandomCopyBytes](https://developer.apple.com/documentation/security/secrandomcopybytes%28_%3A_%3A_%3A%29?language=objc), and Linux/Android's `/dev/urandom` kernel interface ([manual](https://man7.org/linux/man-pages/man4/urandom.4.html)). A source failure is reported rather than falling back to a weak pseudo-random generator.

## Optional SDL3 desktop 2D prototype

The core library remains dependency-free by default. The opt-in `STORM_BUILD_SDL3_DEMO=ON` target fetches the official SDL3 3.4.18 source archive at configure time, verifies its SHA-256, builds SDL statically, and compiles `storm_sdl3_demo`. The desktop CI matrix enables this option so Windows, macOS and Linux compile the pinned integration.

```sh
cmake -S . -B build-sdl3 -DSTORM_BUILD_TESTS=ON -DSTORM_BUILD_SDL3_DEMO=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sdl3 --target storm_sdl3_demo --config Debug --parallel 2
```

Run `build-sdl3/storm_sdl3_demo` on Linux/macOS, or `build-sdl3/Debug/storm_sdl3_demo.exe` for a Visual Studio build. The sample opens a resizable native window, routes SDL events through the bounded StormEngine platform-event queue, updates `InputState`, advances movement using `FixedStepClock`, and draws a colored rectangle using SDL's simple 2D renderer. Move it with WASD/arrows, click to reposition it, use the mouse wheel to resize, and press Escape to exit.

This prototype is desktop-only; configuring it with the Android toolchain is rejected rather than implying that a desktop executable is an APK. SDL's official Android guide requires application/Activity glue and packaging in addition to compiling the native library. A real Android app and device lifecycle smoke tests remain future work. The demo renderer is intentionally temporary and does not define the engine's eventual RHI or 3D backend. The CI build is compile-tested; an interactive GUI smoke test on actual desktops is still required.

Dependency version, provenance and license are recorded in [the SDL3 bootstrap decision](decisions/0001-sdl3-desktop-bootstrap.md) and [third-party notices](../THIRD_PARTY_NOTICES.md).

## Android NDK cross-compilation

The core is currently a native library; this does **not** create a complete Android application or APK. Install Android Studio/SDK and an NDK version supported by your Android Gradle Plugin/toolchain, then set ANDROID_NDK_HOME to that NDK directory.

On a POSIX shell:

1. Configure for a 64-bit ARM device: `cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_STL=c++_shared -DSTORM_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release`
2. Build: `cmake --build build-android --config Release --parallel 2`

Use Android Studio/Gradle with externalNativeBuild when integrating this library into an Android application. Do not hard-code compiler options in Gradle when those options belong to CMake targets. Verify the chosen NDK, Android Gradle Plugin, minimum API level and ABI against the official Android documentation for the application being built.

### Android CI coverage

The GitHub Actions workflow cross-compiles the native core for `arm64-v8a` using Android NDK
`30.0.16248370` (r30 LTS) and `android-26`. The workflow installs that pinned NDK with the SDK
manager if the hosted runner image does not already contain it. This checks the native CMake
toolchain and core sources; it does **not** create an APK, launch an app, or replace smoke tests
on real Android devices. Review the [official NDK release list](https://developer.android.com/ndk/downloads)
before intentionally changing the pinned toolchain version.

## Current scope and limitations

The first milestone verifies the core library and unit tests. An optional SDL3 desktop prototype now opens a window and draws a moving rectangle, but there is no Android activity/APK, production RHI, editor or asset pipeline yet. A successful cross-compile alone is not evidence that the engine is ready for Android gameplay.

## Troubleshooting

- Confirm CMake and compiler versions before debugging source code.
- Delete the build directory when intentionally changing generators or toolchains.
- Do not commit generated build directories, SDK paths, local.properties, binaries, or signing secrets.
- Share the full first compiler error and the configure command when reporting a build issue; later errors may be cascading failures.
