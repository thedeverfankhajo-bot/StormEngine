# Core Game Loop

## Scope

`Storm::GameLoop` is a platform-neutral frame coordinator in the Core module. It consumes an elapsed duration supplied by its caller and invokes a small callback interface in a deterministic order.

It does not create a thread, read a clock, sleep, pump Android/iOS events, own a native surface, render graphics itself, or implement physics. It is not yet connected to the Android NativeActivity entry point or an iOS application.

## Callback order

Each successful `tick(frame_delta, callbacks)` runs these phases:

1. `process_input()` once.
2. `process_events()` once.
3. `fixed_update(fixed_step)` zero or more times, as reported by `FixedStepScheduler`.
4. `variable_update(accepted_frame_delta)` once.
5. `render(interpolation_alpha)` once.

The loop runs on the caller's thread. Implementations of `IGameLoopCallbacks` must obey the runtime's thread-affinity rules and return errors instead of silently swallowing failures.

## Timing contract

- The caller supplies a non-negative elapsed duration, preferably measured by a monotonic platform clock.
- The scheduler caps the accepted frame delta and maximum fixed updates per tick.
- `fixed_update` always receives the configured fixed duration.
- `variable_update` receives the accepted (possibly clamped) frame duration, not the discarded backlog.
- `render` receives the interpolation alpha in the range [0, 1), representing the remainder after fixed-step accounting.
- Clamped frame time and discarded catch-up backlog are returned in `FixedStepAdvance` for diagnostics.

A zero-duration tick still processes input/events, runs the variable update with zero duration, and renders. It performs no fixed update unless a previous fractional remainder plus the supplied delta reaches a full step.

## Lifecycle integration

Core does not depend on `Storm::Platform`. The Android/iOS runtime is responsible for calling `tick` only when its presentation lifecycle permits frame work. On pause/background, and before resuming after suspension, the runtime should call `reset_timing()` so elapsed background time is not simulated as a burst of fixed updates.

The current Android NativeActivity bridge is not wired to `GameLoop`. There is no iOS native adapter yet.

## Error behavior

- A negative frame duration is rejected before any callback is invoked.
- If input or event processing fails, the frame stops before the timing scheduler is advanced.
- If the timing scheduler rejects the duration, the error is returned and simulation/render callbacks are not invoked.
- If a fixed, variable, or render callback fails, the frame stops immediately and the error is returned with the failing phase name and original error code.
- Timing is advanced before fixed-update callbacks begin. If a simulation callback fails partway through the planned fixed updates, some simulation work may already have occurred. Callers must not replay the same elapsed frame duration; they should treat the callback error as an application/runtime failure and apply an explicit recovery policy. `reset_timing()` clears only accumulated timing state; it does not roll back game state.

The loop does not catch arbitrary exceptions from callback implementations. Runtime/plugin boundaries must define their own exception policy.

## Threading and reentrancy

`GameLoop` is single-thread-affine and not internally synchronized. Do not call `tick` concurrently or re-enter the same loop from a callback. The clock, native event pump, and lifecycle gate belong to the platform/runtime integration.

## Current validation

The dedicated host test executable covers configuration validation, callback phase ordering, zero-delta frames, clamped time/backlog reporting, fixed-update failure, variable-update failure, input failure, negative deltas, and timing reset. These tests exercise the platform-neutral coordinator only; they do not validate Android or iOS callback behavior or physical-device frame pacing.
