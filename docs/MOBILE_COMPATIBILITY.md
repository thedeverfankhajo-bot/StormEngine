# Mobile compatibility

StormEngine currently targets Android through the NDK, CMake, EGL, and OpenGL ES. It does not yet provide an iOS/Metal backend.

## Android build/toolchain baseline

| Item | Current baseline |
| --- | --- |
| Android target | API 36 |
| Minimum SDK | API 24 |
| Required graphics feature | OpenGL ES 3.0+ |
| Primary ABI | arm64-v8a |
| Compatibility ABI | armeabi-v7a |
| Emulator/legacy ABIs | x86_64, x86 |
| Android Gradle Plugin | 9.4.0 |
| Gradle | 9.6 |
| Java | 17 |

Google Play requires new apps and updates to target Android 16/API 36 or higher from August 31, 2026.

The Android NDK documentation lists OpenGL ES 3.0 as available from API 18 and recommends querying the runtime GL version/extensions for actual device capabilities.

## Representative device-family matrix

The project uses five representative Android families:

1. Samsung Galaxy
2. Xiaomi / Redmi / POCO
3. Google Pixel
4. OnePlus
5. Motorola

These are hardware families, not five separate operating systems. A family entry is not certification for every model.

## Required physical-device record

For every device validation run, record:

- exact model and product name;
- Android release and API level;
- ABI;
- SoC/GPU;
- OpenGL ES version;
- GL vendor and renderer strings;
- StormEngine commit;
- APK variant and build tools;
- surface creation/destruction;
- pause/resume;
- rotation/configuration change;
- context-loss/recreation behavior;
- shader compilation;
- texture upload and mip behavior;
- buffer creation/update/destruction;
- basic 3D draw;
- visible corruption or GL/EGL errors;
- sustained stability.

## Lifecycle test sequence

1. Launch with a valid SurfaceView.
2. Wait for a stable 3D frame.
3. Background and foreground the Activity.
4. Destroy and recreate the surface.
5. Rotate/configure the device.
6. Repeat pause/resume and surface recreation.
7. Exercise resource creation and destruction.
8. If the driver exposes context loss, verify CPU-side resources restore into the new context.
9. Record the final GL vendor/renderer and engine commit.

The native window API exposes width/height and explicit acquire/release ownership, so the renderer must never retain a Java Surface or ANativeWindow beyond its valid lifecycle.

## Graphics compatibility

The manifest declares GLES 3.0 as a required feature. The engine should still query actual runtime capabilities before using optional GLES features.

EGL configuration selection uses a 16-bit minimum depth request rather than requiring a 24-bit depth buffer, reducing unnecessary rejection on devices whose compatible configurations expose only 16-bit depth.

## Termux

Termux is a first-class native development environment for the portable core.

    pkg update
    pkg install clang cmake ninja make git
    ./scripts/termux-build.sh

The helper validates CMake 3.23+, compiler selection, generator selection, build options, and parallelism. It does not download or execute remote code.

## Compatibility interpretation

An APK that compiles for all configured ABIs demonstrates packaging/build coverage only. It does not prove that every ABI, GPU driver, vendor extension, Android version, or physical device passes runtime validation.

Failures should be recorded with exact hardware/software information so driver-specific behavior can be distinguished from engine bugs.
