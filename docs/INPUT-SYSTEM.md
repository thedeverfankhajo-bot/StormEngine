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
Android/iOS callbacks must be serialized onto its owning thread. At the start
of each input frame, call `begin_frame()`; then submit the events for that
frame with `process()`. The ordered events are exposed by `frame_events()`;
active positions and accumulated deltas are exposed by `active_pointers()`.

Every event carries the currently attached surface generation and a
monotonic timestamp. This layer does not read a clock, convert pixels to
logical points, or own native surface handles.

Call `cancel_all(timestamp_ns)` when interactive focus is lost. Call
`detach_surface(generation, timestamp_ns)` when the surface is destroyed.
Cancellation events are appended before active state is cleared. If the event
buffer cannot hold all cancellation events, the operation fails without
mutating active pointers; begin a new frame and retry.

## Limits

Defaults are 10 active pointers and 256 events per frame. Hard limits are 64
active pointers and 4096 events per frame. Event capacity must be at least the
pointer capacity so that all active pointers can be cancelled. Both vectors
are reserved at creation; processing within configured limits does not require
vector growth. Capacity exhaustion returns a typed error rather than silently
dropping events.

## Limitations

No Android MotionEvent or UIKit callback adapters, gesture recognizers,
virtual controls, action mapping, sensor/haptics integration, or physical-device
input tests exist yet. The API is experimental and has no compatibility
guarantee.

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
