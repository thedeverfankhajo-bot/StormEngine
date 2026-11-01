# Engine Lifecycle Contract

## Scope

This document describes the platform-neutral lifecycle state model implemented in `Storm::Platform::LifecycleStateMachine`. It does not claim to implement Android Activity callbacks, iOS UIKit delegates, a native application entry point, window/surface creation, memory-warning delivery, or a game loop.

Each instance represents one native presentation lifecycle stream. An Android integration will normally associate it with an Activity. An iOS integration that uses scenes should associate one instance with each `UIScene`; applications without scene support may associate it with the app-level lifecycle stream. This avoids incorrectly collapsing independent iOS scene states into one global state.

## States

| State | Meaning | Game-loop policy |
|---|---|---|
| `Created` | No foreground/background lifecycle transition has been reported yet | Do not run |
| `ForegroundInactive` | The presentation is in the foreground but is not currently active/interactive | Do not run |
| `ForegroundActive` | The presentation is active and interactive | May run |
| `Background` | The presentation is not in the foreground | Do not run |
| `Terminating` | The native owner has reported termination | Do not run; no later transition is valid |

`is_foreground()` is true in both foreground states. `can_run_game_loop()` is true only in `ForegroundActive`. This is a conservative engine policy; it is not a statement that Android or iOS require all rendering to stop during every inactive interval.

## Allowed transitions

| Current | Allowed next states |
|---|---|
| `Created` | `ForegroundInactive`, `Background`, `Terminating` |
| `ForegroundInactive` | `ForegroundActive`, `Background`, `Terminating` |
| `ForegroundActive` | `ForegroundInactive`, `Terminating` |
| `Background` | `ForegroundInactive`, `Terminating` |
| `Terminating` | None |

A transition to the current state is idempotent and succeeds with `changed == false`. An invalid transition returns `ErrorCode::InvalidState` and leaves the state unchanged. Termination is allowed from any non-terminating state because the OS may request termination without a complete foreground/background round trip.

A launch that starts in the background is represented by `Created -> Background`. On a foreground return, the native adapter reports `Background -> ForegroundInactive -> ForegroundActive`.

## Native callback mapping to be implemented

These are mapping guidelines for future adapters, not code that exists in this repository yet.

### Android

For a single-Activity runtime, an adapter can normalize `onStart` to `ForegroundInactive`, `onResume` to `ForegroundActive`, `onPause` to `ForegroundInactive`, and `onStop` to `Background`. `onCreate` performs native setup; it should not be treated as proof that the Activity is already interactive. `onDestroy` is an Activity lifecycle callback and must not automatically be treated as a reliable process-termination notification.

### iOS

For scene-based UIKit applications, map `sceneWillEnterForeground` to `ForegroundInactive`, `sceneDidBecomeActive` to `ForegroundActive`, `sceneWillResignActive` to `ForegroundInactive`, and `sceneDidEnterBackground` to `Background`. Keep independent state machines for independent scenes. For apps without scene support, map the corresponding app-delegate lifecycle notifications to one app-level instance.

UIKit may launch a scene directly into the background for background event processing. Such an owner can begin with `Created -> Background` and later transition to the foreground. A termination callback is best-effort and must not be the only place where important state is saved or resources are made safe.

## Threading contract

`LifecycleStateMachine` is intentionally not internally synchronized. The native adapter must serialize transitions on its native lifecycle callback queue. If the engine consumes lifecycle information on another thread, a later integration must forward transitions through an explicitly synchronized event queue; it must not call this class concurrently.

## Separate concerns

The following are not lifecycle states and will have separate contracts when implemented:

- Window or render-surface created, resized, lost, and recreated.
- Memory pressure and low-memory notifications.
- Orientation and display metrics changes.
- Audio interruptions and audio-focus changes.
- Device termination and process death diagnostics.

A surface can be recreated while the application remains active. Therefore surface/resource lifetime must not be inferred solely from this application/presentation lifecycle state.

## Current validation

The state machine and its transition rules are covered by host unit tests. GCC 14.2 and Clang 17 builds, and a Clang AddressSanitizer/UndefinedBehaviorSanitizer test build, passed in the current engineering environment. No Android SDK/NDK or Xcode/UIKit adapter build has been run; neither Android nor iOS compatibility is verified.
