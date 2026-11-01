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

## Lifecycle event queue

`Storm::Platform::LifecycleEventQueue` provides the first thread boundary for future native adapters. It is a fixed-capacity ring buffer of 64 state events and performs no heap allocation when enqueuing or dequeuing.

- `try_push()` returns `Enqueued`, `Full`, or `Closed` explicitly. A full or closed queue never silently drops an event while reporting success.
- `try_pop()` returns the oldest event, preserving FIFO order across successful enqueues.
- `close()` rejects new events but leaves already queued events available for draining.
- Multiple producers are serialized by a mutex. One engine-thread consumer is the intended usage model.

The native callback must not block waiting for capacity. If `try_push()` returns `Full`, the adapter must surface a diagnostic and apply an explicit recovery policy; it must not pretend the transition was delivered. Queue overflow is a serious lifecycle-delivery failure and should be treated as such by a real platform adapter. The capacity of 64 is an initial bounded default, not a proven universal limit.

The engine thread should dequeue each event and pass it to `LifecycleStateMachine::transition_to()`. The queue preserves enqueue order; the state machine remains the authority that validates legal state transitions.

## Threading contract

`LifecycleStateMachine` is intentionally not internally synchronized. Native adapters should enqueue callback-derived state changes instead of calling the state machine concurrently. The queue serializes producers and is designed for one consumer on the engine thread. If more than one consumer is introduced, ordering and ownership semantics must be redesigned and tested explicitly.

## Surface lifecycle contract

`Storm::Platform::SurfaceLifecycle` is a separate, platform-neutral contract for one render-surface stream. It deliberately does not change `LifecycleStateMachine`: an application can remain active while a native surface is destroyed and recreated.

A `SurfaceDescriptor` contains:

- A non-zero opaque `native_id` supplied by the adapter. The descriptor does not own a native object and must never contain a borrowed raw pointer.
- A monotonically increasing `generation` for each successful attach on this lifecycle instance.
- A pixel `width` and `height`, both greater than zero.
- A finite positive `pixels_per_logical_unit` value for display-density conversion.

`attach()` rejects invalid descriptors and a second attach while a surface is already active. `resize()` updates extent and scale without changing the generation. `detach()` clears the active descriptor only when both identity and generation match. A callback with an old identity or generation is rejected with `ErrorCode::InvalidState`, so a delayed callback from a destroyed surface cannot mutate a replacement surface. Generation exhaustion is reported as `ErrorCode::ResourceExhausted` rather than wrapping.

The contract is single-thread-affine, like the lifecycle state machine. A native adapter must serialize its surface callbacks before calling it. The adapter remains responsible for native handle ownership and must stop using a surface after detach. The generation is a stale-callback guard, not a GPU-resource synchronization primitive; render-backend fences and resource retirement still need a separate design.

A platform integration should create one surface lifecycle instance per independently managed presentation surface. On recreation it must detach the old identity before attaching the new one. It must not infer that a valid surface exists merely because the app lifecycle is foreground-active.

## Separate concerns

The following remain separate contracts or future integrations:

- Memory pressure and low-memory notifications.
- Orientation and display metrics changes beyond the extent and scale reported through surface callbacks.
- Audio interruptions and audio-focus changes.
- Device termination and process death diagnostics.
- GPU resource fences, surface-loss recovery, and swapchain/drawable recreation.

## Current validation

The lifecycle state machine and event queue are covered by the foundation host tests, including FIFO behavior, full/closed handling, and concurrent multi-producer delivery (16,000 enqueued events in the stress test). The surface lifecycle contract has a separate host test executable covering attach validation, generation changes, resize, detach, recreation, and stale callbacks. GCC 14.2 and Clang 17 host builds passed. No Android SDK/NDK or Xcode/UIKit adapter build has been run; neither Android nor iOS compatibility is verified.
