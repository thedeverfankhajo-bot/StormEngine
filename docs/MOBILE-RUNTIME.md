# Mobile Runtime Frame Driver

## Purpose and boundaries

`Storm::Runtime::MobileFrameDriver` connects the existing platform lifecycle and surface contracts to the platform-neutral `Storm::GameLoop`. The Runtime target depends on Core and Platform; Core itself remains independent of Android, UIKit, and the Platform module.

The driver is not a native application loop. It does not create a clock, block or sleep, own a frame-pacing API, start threads, acquire native surfaces, or render. The caller samples a monotonic timestamp from the appropriate mobile frame source and invokes `tick()` with the application-owned `IGameLoopCallbacks` implementation.

## Frame eligibility

A frame is eligible only when both conditions hold:

1. The lifecycle state is `ForegroundActive`.
2. `SurfaceLifecycle::current()` contains an attached, validated surface descriptor.

When either condition is false, `tick()` returns a typed skip disposition and does not invoke input, event, update, or render callbacks. It also resets accumulated fixed-step timing.

## Timing and environment changes

The first eligible frame uses a zero duration. The driver also resets timing when it detects:

- A new lifecycle revision. `LifecycleStateMachine::revision()` increments for each successful non-idempotent transition, allowing the driver to detect a background/resume sequence that happened entirely between two frame calls.
- A new surface generation. `SurfaceLifecycle` increments its generation on each successful attach; a detach/re-attach is therefore detected even if no frame ran while the surface was absent.
- An explicit `reset_timing()` call from the owning runtime.

A resize that keeps the same surface generation does not reset the simulation clock. The caller may use either `tick()` with `std::chrono::steady_clock::time_point` or `tick_at_monotonic_nanoseconds()` with a platform-native monotonic timestamp. The native overload deliberately treats the clock epoch as opaque; timestamps must be non-decreasing and must come from one clock source for the duration of an active timing period. Do not mix native clock sources or wall-clock timestamps without first calling `reset_timing()`. Timestamp regressions return `InvalidArgument`; intervals that cannot fit in signed nanoseconds return `ResourceExhausted`, and both errors reset timing.

The timestamp is recorded before game callbacks run. A callback failure is returned unchanged from Core and that elapsed frame interval is not replayed by the next call.

## Native timestamp boundary

`tick_at_monotonic_nanoseconds()` allows a native frame callback to pass its own monotonic timestamp without converting its clock epoch to `std::chrono::steady_clock`. For example, Android's `AChoreographer_frameCallback64` supplies `frameTimeNanos` in the `CLOCK_MONOTONIC` time base, while iOS `CADisplayLink` supplies monotonic Core Animation timestamps that can be converted to nanoseconds. The caller remains responsible for acquiring and scheduling those callbacks. This API does not create or register an Android Choreographer or iOS CADisplayLink source, and neither native adapter is connected yet.

## Accepted native frame-source design

The platform scheduling direction is recorded in [ADR-0008](ADR-0008-NATIVE-FRAME-SOURCES.md). It is an architectural decision, not an implementation claim.

- Android API 24 and newer: use the NDK `AChoreographer` from the NativeActivity app thread, which is already the thread servicing the native looper. Resolve API-24+ symbols at runtime so the minSdk 21 binary does not acquire an unconditional API-24 link dependency. Prefer the 64-bit callback on API 29 and newer. The current Android package is arm64-v8a only; any future 32-bit ABI requires explicit handling of the legacy callback timestamp width before it can be enabled.
- Android API 21–23: preserve the minSdk with a monotonic-deadline fallback integrated into `ALooper_pollOnce`. This is timer-paced and is explicitly not VSync. Its nominal period is configurable; the initial fallback target is 60 Hz, not a device-wide frame-rate guarantee.
- iOS 13+: use `CADisplayLink` registered on the main run loop in common modes. Keep the platform callbacks, lifecycle state, surface contract, and first Runtime consumer on the same thread. Treat frame-rate preferences as advisory; use the system's supported cadence by default.
- Both sources are one-shot/re-arm or run-loop driven. They are active only while the app is foreground-active and a surface is attached. When eligibility ends, stop requesting additional frames and reset `MobileFrameDriver` timing before resumption.
- Feed one consistent monotonic time base into `tick_at_monotonic_nanoseconds()`. Do not mix wall-clock timestamps or switch clock sources during an active timing period.

No native frame source, callback registration, renderer, or GameLoop integration is added by this decision record. The existing Android event pump and iOS host remain unconnected to `MobileFrameDriver` until the platform adapters and an application-owned `IGameLoopCallbacks` implementation are added and tested.

## Ownership and threading

The lifecycle state machine and surface contract remain owned by the platform/runtime host and must outlive the driver. The driver owns its `GameLoop` and stores non-owning references to those contracts. It is single-thread-affine: lifecycle/surface updates and `tick()` must be serialized on the same owning thread. A future dedicated render/engine thread must receive those changes through an explicitly bounded handoff rather than concurrently reading these unsynchronized contracts.

## Frame timestamp callback and action input

Before invoking the Core loop on each eligible frame, `MobileFrameDriver`
calls `IGameLoopCallbacks::begin_frame()` with the same monotonic timestamp
used to calculate the frame delta. The default callback is a no-op, so
existing application callback implementations remain source-compatible. A
callback error aborts the frame; the attempted timestamp has already been
recorded and is not replayed.

`GestureActionGameLoopAdapter` uses this hook to pass an absolute frame time
to `GestureActionInputPipeline`. This is essential for time-based gestures:
LongPress may be recognized on a frame with no new pointer events. Negative
timestamps are rejected at this adapter boundary because pointer and gesture
timestamps are represented as unsigned nanoseconds. The adapter runs gesture
recognition and action mapping before the delegated application
`process_input()` callback, then clears per-frame touch events/deltas on
every exit path. Non-empty action batches are delivered synchronously to an
application-owned `IGameActionFrameConsumer`; the consumer must not retain
the non-owning span after its callback returns.

This connects the portable touch, gesture, and action layers to a gameplay
consumer contract. It does not instantiate a gameplay consumer in the Android
or iOS host shells, create a native frame source, or claim physical-device
validation.

## Current integration limits

The driver is an orchestration component, not yet wired into the Android NativeActivity event pump or the iOS delegate host application. The Android bridge still runs an event pump and the iOS delegate still exposes overridable lifecycle hooks. No Android Choreographer, iOS CADisplayLink, native frame pacing, renderer, physics, installable app target, or physical-device run is included by this component.

## CI validation evidence

The successful six-job workflow [41307026111](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41307026111) for commit `4a3515c8cd8da92012edd018ed1a51045bad9269` built the Runtime static library for Android arm64-v8a and iOS device arm64. Eleven dedicated host cases passed in the Runtime CTest executable under GCC, Clang, Apple Clang, and Clang ASan/UBSan. This verifies the orchestration contract and target compilation only; neither platform adapter calls this driver yet, and no native frame source or physical-device execution is included.
