# Mobile Input Foundation

## Scope

`Storm::Input` is a portable C++20 input-ingestion target. It is independent
of Android, UIKit, Core, Scene, and rendering APIs. Native adapters translate
OS pointer callbacks into this contract.

It supports Down, Move, Up, and Cancel events for multiple stable pointer IDs,
logical point coordinates, caller-supplied monotonic nanosecond timestamps,
per-frame accumulated movement deltas, surface-generation validation,
explicit cancellation, and bounded active-pointer/event capacities.

## Thread and frame contract

`TouchInput` is single-thread-affine and is not internally synchronized.
Android/iOS callbacks must be serialized onto its owning thread. The low-level
API exposes `begin_frame()` to clear old event/delta data before a caller-owned
event batch. Native callbacks are asynchronous relative to display frames, so
do not clear the buffer immediately before the gameplay consumer reads it.
Allow native callbacks to accumulate events until the next input phase; the
Runtime adapter clears the per-frame data after consumption. The ordered
events are exposed by `frame_events()`; active positions and accumulated
deltas are exposed by `active_pointers()`.

Every event carries the currently attached surface generation and a
monotonic timestamp. This layer does not read a clock, convert pixels to
logical points, or own native surface handles. Native callback batches use
process_batch(); a rejected batch drops partial frame events and cancels all
active pointers so malformed OS input cannot strand a pointer in the engine.

Call `cancel_all(timestamp_ns)` when interactive focus is lost. Call
`detach_surface(generation, timestamp_ns)` when the surface is destroyed.
Cancellation events are appended before active state is cleared. If the event
buffer cannot hold all cancellation events, the operation fails without
mutating active pointers; begin a new frame and retry.

## Game-loop consumption

`Storm::Runtime::TouchInputGameLoopAdapter` decorates an application-owned
`IGameLoopCallbacks` implementation and an `ITouchInputFrameConsumer`.
During `GameLoop::process_input()`, it supplies a non-owning
`TouchInputFrameView` containing the ordered pointer events, current active
pointer snapshots, and optional surface generation. The touch consumer runs
before the application's existing `process_input()` callback; the remaining
events, fixed-update, variable-update, and render callbacks are forwarded
unchanged.

The frame view and its spans are valid only for the duration of
`process_touch_input()`; consumers must copy data they need to retain. After
the consumer returns, the adapter clears frame events and movement deltas even
when the consumer reports an error, preventing stale transitions from being
replayed on a later frame. The adapter does not own the input object, surface,
clock, or native callback pump. The input object, consumer, and delegated
callbacks must outlive the adapter and be accessed on the same serialized
thread.

## Limits

Defaults are 10 active pointers and 256 events per frame. Hard limits are 64
active pointers and 4096 events per frame. Event capacity must be at least the
pointer capacity so that all active pointers can be cancelled. Both vectors
are reserved at creation; processing within configured limits does not require
vector growth. Capacity exhaustion returns a typed error rather than silently
dropping events.

## Native callback adapters

The Android NativeActivity host maps NDK MotionEvent Down/PointerDown, Move,
PointerUp/Up, and Cancel actions into bounded pointer batches. It accepts
unknown, finger, stylus, and eraser tools and ignores mouse/palm/non-touch
motion sources. NDK X/Y values are pixels relative to the receiving window,
so the adapter divides by the active surface's validated
pixels-per-logical-unit scale. If the density scale changes, active pointers
are cancelled before the new logical-coordinate scale is applied. It forwards the NDK
event timestamp in the System.nanoTime nanosecond time base and uses pointer IDs,
not transient pointer indices. See the [Android NDK Input reference](https://developer.android.com/ndk/reference/group/input).

The iOS host view enables multi-touch and maps UIKit
`touchesBegan`, `touchesMoved`, `touchesEnded`, and `touchesCancelled` callbacks.
It uses view-local point coordinates, UITouch object identity for pointer IDs,
and the enclosing UIEvent timestamp converted from seconds since system
startup to nanoseconds. See [Apple's UIEvent timestamp documentation](https://developer.apple.com/documentation/uikit/uievent/timestamp).

Both adapters bind events to the current surface generation and cancel active
pointers on Android focus loss or surface detachment. Callback batches are
single-thread-affine and pass through `TouchInput::process_batch`; a rejected
batch clears partial events and cancels pointers that were already active.

## Limitations

The native callbacks update a portable TouchInput instance owned by each host;
they are not yet exposed to Core game-loop phases or gameplay systems. Gesture
recognizers, virtual controls, action mapping, sensor/haptics integration, and
physical-device input tests do not exist yet. The API is experimental and has
no compatibility guarantee. See [ADR-0013](ADR-0013-NATIVE-TOUCH-ADAPTERS.md) for the native adapter boundary and clock/coordinate decisions.

## Example

```cpp
auto created = Storm::Input::TouchInput::create();
if (!created) {
    // Handle initialization failure.
}
auto input = std::move(created).value();
auto attached = input.attach_surface(surface_generation);
if (!attached) {
    // Handle an invalid generation.
}
input.begin_frame();
auto result = input.process(Storm::Input::PointerEvent{
    pointer_id, Storm::Input::PointerPhase::Down,
    logical_x, logical_y, monotonic_timestamp_ns, surface_generation
});
if (!result) {
    // Handle invalid input or capacity exhaustion.
}
```
