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

## Fixed-step timing utility

`Storm::FixedStepScheduler` is a platform-neutral Core utility for deterministic update scheduling. It receives an elapsed `std::chrono::nanoseconds` duration from its caller; it does not read a clock, sleep, render, or invoke update callbacks.

- `fixed_step` must be positive and defines the duration represented by one simulation update.
- `maximum_frame_delta` must be positive and caps elapsed time accepted from one frame.
- `maximum_steps_per_frame` must be positive and caps catch-up work in one call.
- `advance()` reports the number of fixed updates the caller may execute, the accepted duration, the clamped duration, any whole-step backlog discarded after the catch-up limit, and an interpolation alpha for the remaining sub-step fraction.
- If the accumulator arithmetic would overflow its duration representation, `advance()` returns `ErrorCode::ResourceExhausted` and leaves its previous state unchanged.
- Negative frame deltas and invalid configurations return `ErrorCode::InvalidArgument`.
- `reset()` clears the accumulated remainder, for example after a lifecycle transition where elapsed background time must not be simulated.

The default configuration uses a 16,666,667 ns fixed step, a 250 ms maximum accepted frame delta, and at most eight updates per frame. These are conservative starting defaults, not a device performance guarantee or a physics-determinism guarantee.

The scheduler intentionally separates time accounting from simulation. A future game loop must decide when to call it, execute exactly the reported number of fixed updates, define what happens when a scene pauses, and avoid applying background elapsed time unintentionally. The scheduler does not currently control the Android NativeActivity event pump.

## Native callback mapping and implementation status

Android lifecycle normalization is implemented in Storm::Platform::Android. iOS scene-event normalization is implemented in Storm::Platform::IOS; the actual UIKit delegate bridge remains planned. The mappings below distinguish event policy from OS callback integration.

### Android

For a single-Activity runtime, an adapter can normalize `onStart` to `ForegroundInactive`, `onResume` to `ForegroundActive`, `onPause` to `ForegroundInactive`, and `onStop` to `Background`. `onCreate` performs native setup; it should not be treated as proof that the Activity is already interactive. `onDestroy` is an Activity lifecycle callback and must not automatically be treated as a reliable process-termination notification.

### iOS

For scene-based UIKit applications, create one lifecycle state machine per UIScene and pass normalized callbacks to Storm::Platform::IOS::apply_scene_event():

- sceneWillEnterForeground -> SceneEvent::WillEnterForeground -> ForegroundInactive
- sceneDidBecomeActive -> SceneEvent::DidBecomeActive -> ForegroundActive
- sceneWillResignActive -> SceneEvent::WillResignActive -> ForegroundInactive
- sceneDidEnterBackground -> SceneEvent::DidEnterBackground -> Background

For apps without scene support, map the corresponding app-delegate lifecycle notifications to one app-level instance. The current C++ adapter implements this normalization policy, but does not implement or register a UIKit delegate. See [iOS Lifecycle Event Adapter](IOS-LIFECYCLE.md).

UIKit may launch a scene directly into the background for background event processing. Such an owner can begin with Created -> Background and later transition to the foreground. The app-scoped termination callback can be forwarded with apply_application_event(), but it is best-effort and must not be the only place where important state is saved or resources are made safe.

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

The lifecycle state machine and event queue are covered by the foundation host tests, including FIFO behavior, full/closed handling, and concurrent multi-producer delivery (16,000 enqueued events in the stress test). The surface lifecycle contract has a separate host test executable covering attach validation, generation changes, resize, detach, recreation, and stale callbacks. The fixed-step scheduler has dedicated tests for configuration validation, partial accumulation, fixed-step emission, frame-delta clamping, catch-up limits, state preservation after invalid input/overflow, and reset.

No Android SDK/NDK or Xcode/UIKit adapter runtime tests have been run on physical devices. The Android NativeActivity bridge is cross-compiled and linked in CI only. The iOS event-normalization policy has host tests, but UIKit callback delivery and native lifecycle integration remain unimplemented.
