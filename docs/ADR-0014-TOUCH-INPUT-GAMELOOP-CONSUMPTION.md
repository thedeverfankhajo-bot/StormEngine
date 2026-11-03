# ADR-0014: Frame-Scoped Touch Input Consumption

- Status: Accepted
- Date: 2026-11-03
- Scope: Portable Runtime-to-GameLoop touch input handoff

## Context

The Android NativeActivity and iOS UIKit hosts now translate native pointer
callbacks into the platform-neutral `Storm::Input::TouchInput` state machine.
That state is single-thread-affine and owned by each host. Core's
`IGameLoopCallbacks::process_input()` intentionally has no platform-specific
parameters, so the native input state otherwise has no supported path into
gameplay code.

Native touch callbacks arrive asynchronously between display frames. Clearing
the input event buffer at the beginning of a display callback can discard
events that arrived since the previous frame.

## Decision

Add `Storm::Runtime::TouchInputGameLoopAdapter`, an optional decorator around
an application-owned `IGameLoopCallbacks`, `TouchInput`, and an
`ITouchInputFrameConsumer`.

For each eligible GameLoop input phase, the adapter:

1. Presents a non-owning `TouchInputFrameView` containing ordered events,
   active pointer snapshots, and the optional surface generation.
2. Invokes the touch consumer before the application's existing
   `process_input()` callback.
3. Clears per-frame events and pointer deltas after the touch consumer returns,
   including when it returns an error, so stale transitions are not replayed.
4. Forwards events, fixed updates, variable updates, and render callbacks to
   the wrapped application unchanged.

The adapter is single-thread-affine and borrows all three dependencies. The
consumer may inspect the supplied spans only during the callback; it must copy
any data it needs to retain. Core and the standalone Input target remain
independent from one another.

## Consequences and validation boundary

This creates a supported, testable handoff from host-owned touch state into
the Core input phase without introducing Android or UIKit headers into Core.
Host tests cover ordering, active-pointer/delta delivery, cancellation,
forwarding, and no-replay behavior on callback errors.

The adapter does not create a native frame source, connect the existing host
objects to an application-owned GameLoop, implement gestures/actions, or prove
physical-device input delivery. Those remain separate integration steps.
