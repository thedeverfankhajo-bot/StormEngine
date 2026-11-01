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

A resize that keeps the same surface generation does not reset the simulation clock. The caller must continue to provide monotonic `std::chrono::steady_clock::time_point` values. A timestamp regression returns `InvalidArgument` and resets the timing state; an elapsed interval that cannot be represented as nanoseconds returns `ResourceExhausted` and also resets timing.

The timestamp is recorded before game callbacks run. A callback failure is returned unchanged from Core and that elapsed frame interval is not replayed by the next call.

## Ownership and threading

The lifecycle state machine and surface contract remain owned by the platform/runtime host and must outlive the driver. The driver owns its `GameLoop` and stores non-owning references to those contracts. It is single-thread-affine: lifecycle/surface updates and `tick()` must be serialized on the same owning thread. A future dedicated render/engine thread must receive those changes through an explicitly bounded handoff rather than concurrently reading these unsynchronized contracts.

## Current integration limits

The driver is an orchestration component, not yet wired into the Android NativeActivity event pump or the iOS delegate host application. The Android bridge still runs an event pump and the iOS delegate still exposes overridable lifecycle hooks. No Android Choreographer, iOS CADisplayLink, native frame pacing, renderer, physics, installable app target, or physical-device run is included by this component.

## CI validation evidence

The successful six-job workflow [41307026111](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41307026111) for commit `4a3515c8cd8da92012edd018ed1a51045bad9269` built the Runtime static library for Android arm64-v8a and iOS device arm64. Eleven dedicated host cases passed in the Runtime CTest executable under GCC, Clang, Apple Clang, and Clang ASan/UBSan. This verifies the orchestration contract and target compilation only; neither platform adapter calls this driver yet, and no native frame source or physical-device execution is included.
