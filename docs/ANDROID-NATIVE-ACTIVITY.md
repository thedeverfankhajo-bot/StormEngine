# Android NativeActivity Lifecycle Bridge

Last reviewed: 2026-11-01

## Scope and status

StormEngine has an Android-specific NativeActivity entry point built with the Android NDK's android_native_app_glue. It handles lifecycle, native-window metadata, and translates NDK MotionEvent touch callbacks into the portable Storm::Input::TouchInput contract. It is not an Android game runtime.

The bridge currently:

- Receives NativeActivity START, RESUME, PAUSE, STOP, focus, and destroy commands.
- Converts lifecycle commands into the platform-neutral LifecycleStateMachine.
- Attaches, resizes, and detaches the current window through SurfaceLifecycle.
- Queries only the window pixel extent and configuration density.
- Generates opaque, monotonically increasing surface identity values. It does not store an ANativeWindow pointer in SurfaceDescriptor.
- Registers NativeActivity onInputEvent and maps Down/PointerDown, Move, PointerUp/Up, and Cancel into bounded pointer batches.
- Converts window-relative pixel coordinates to logical units using the active surface density; uses the NDK event nanosecond timestamp and stable pointer IDs.
- Ignores mouse/palm/non-touch tool sources while accepting unknown, finger, stylus, and eraser tools.
- Cancels active pointers on focus loss, surface detachment, and surface geometry/density changes.
- Uses the NativeActivity glue event pump while no game loop exists.

The transition policy is host-tested. The Android shared library is compiled and linked by the Android NDK CI job. No Android device or emulator execution has yet been performed.

## Native app integration

The Android CMake target is storm_android_native_activity; its shared object is named libstorm_android_native_activity.so. The NativeActivity manifest metadata uses the library name without the lib prefix or .so suffix.

A consuming Android application can merge an activity declaration equivalent to:

~~~xml
<activity
    android:name="android.app.NativeActivity"
    android:configChanges="orientation|keyboardHidden|screenSize|uiMode"
    android:exported="true"
    android:label="StormEngine">
    <meta-data
        android:name="android.app.lib_name"
        android:value="storm_android_native_activity" />
    <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
    </intent-filter>
</activity>
~~~

The repository now includes a Gradle application module that packages this NativeActivity bridge into an arm64 debug APK. It does not yet generate an AAB, configure release signing, or provide a game/renderer implementation. See [Mobile App Hosts](MOBILE-APP-HOSTS.md).

## Build behavior

When CMake is configured with the Android NDK toolchain:

- C is enabled for the NDK-provided android_native_app_glue.c.
- storm_platform includes the portable Android lifecycle event policy.
- storm_android_native_activity links the NativeActivity glue, Storm::Platform, android, and log.
- The NDK glue source is consumed from the installed NDK; it is not copied into or vendored by this repository.

The CI configuration currently uses ANDROID_ABI=arm64-v8a and ANDROID_PLATFORM=android-21 for this compile job. The API level is a CI toolchain setting only, not a declared product minimum SDK.

## Lifecycle mapping

| Native App Glue command | Normalized event | Resulting policy |
|---|---|---|
| APP_CMD_START | Started | Created/Background becomes ForegroundInactive |
| APP_CMD_RESUME | Resumed | ForegroundInactive; repeated resume is idempotent |
| APP_CMD_PAUSE | Paused | ForegroundActive becomes ForegroundInactive |
| APP_CMD_STOP | Stopped | Transitions to Background; passes through ForegroundInactive if needed |
| APP_CMD_GAINED_FOCUS | FocusGained | ForegroundActive; recovers through ForegroundInactive if needed |
| APP_CMD_LOST_FOCUS | FocusLost | ForegroundActive becomes ForegroundInactive |
| APP_CMD_DESTROY | Destroyed | Transitions to terminal Terminating state |

Repeated events are intentionally idempotent where the native lifecycle can report redundant state. Events that would revive a terminating lifecycle return an explicit error.

The state machine and adapter are single-thread-affine. Native App Glue dispatches these callbacks to the native app thread; the adapter must continue to be called serially on that thread. The existing bounded lifecycle queue is not used here because this bridge does not introduce a second producer thread.

## Surface lifetime and ownership

On APP_CMD_INIT_WINDOW, the bridge validates the current ANativeWindow dimensions and attaches a new SurfaceDescriptor. On resize/configuration changes it updates the current generation. On APP_CMD_TERM_WINDOW and destroy it detaches that generation.

The adapter does not acquire, release, or retain the ANativeWindow pointer. The pointer is borrowed from android_app only while handling the relevant callback. Any future renderer must establish its own explicit native-window ownership and GPU-resource synchronization contract before using the surface after a callback returns.

## Not implemented by this bridge

- A game loop, frame pacing, or gameplay update scheduler.
- Rendering, swapchain creation, Vulkan, OpenGL ES, or GPU resource management.
- Core game-loop/gameplay input consumption, gesture recognition, action mapping, or sensor integration.
- Android Java/Kotlin UI, custom JNI APIs, permissions, audio, or background task policy.
- Gradle project generation, APK/AAB packaging, signing, deployment, or Play distribution.
- Device runtime, lifecycle stress, rotation, low-memory, or surface-recreation validation on physical hardware.
- The corresponding UIKit/Objective-C++ iOS adapter.

## References

- [Android NDK: Native App Glue source and command contract](https://android.googlesource.com/platform/ndk/+/master/sources/android/native_app_glue/android_native_app_glue.h)
- [Android NDK: Native Window reference](https://developer.android.com/ndk/reference/group/a-native-window)
- [Android NDK: Configuration reference](https://developer.android.com/ndk/reference/group/configuration)
