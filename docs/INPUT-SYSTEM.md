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
single-thread-affine and pass through `TouchInput::process_batch`. Separate
native callbacks accumulate in the same pending input frame; neither adapter
calls `begin_frame()` before each OS callback. The
`TouchInputGameLoopAdapter` clears events and deltas only after the gameplay
consumer has observed the frame. A rejected batch clears partial transitions
and cancels pointers that were already active. When a lifecycle cancellation
cannot fit in the bounded event buffer, older buffered transitions are
discarded only to guarantee delivery of all required cancellation events.

## Game action mapping

`InputActionMap` maps recognized `GestureEvent` records to caller-defined,
non-zero 32-bit `InputActionId` values. A binding is one
`(GestureType, InputActionId)` pair; one gesture can intentionally trigger
multiple distinct actions, and output order follows input gesture order then
binding declaration order. Duplicate pairs are rejected.

Action phases are derived consistently from gesture lifecycle:
`DragBegin`/`PanBegin`/`PinchBegin` become `Started`; gesture updates and
one-shot gestures become `Performed`; normal Drag/Pan/Pinch endings become
`Completed`; cancelled endings and explicit cancellation become `Cancelled`.
Each action event retains the original gesture payload and pointer identities.

Bindings may be replaced at runtime with `replace_bindings()`. Replacement is
validated and built separately before swapping into place, so an invalid
replacement or allocation failure leaves the previous binding set unchanged.
An empty binding list intentionally disables all action mappings.

The map is single-thread-affine. Binding replacement is an explicit
configuration operation; `update()` clears and fills a pre-reserved output
buffer and does not allocate within its configured limits. Defaults are 128
bindings, 4096 gesture events per update, and 256 action events per update.
Hard limits are 1024 bindings, 4096 gesture events, and 8192 action events.
Inputs are validated for known gesture types, finite numeric values, positive
scale, cancellation consistency, and monotonic timestamps across updates.
An invalid batch or output-capacity overflow returns a typed error and exposes
no partial action output. The result span is valid only until the next update
or binding replacement.

This is a portable mapping primitive, not yet a complete action-context
system: context stacking, device-specific bindings, configurable dead zones,
virtual joysticks/buttons, and persistent user-configurable mappings remain
unimplemented.

## Runtime gesture/action pipeline

Storm::Runtime::GestureActionInputPipeline composes the existing
GestureRecognizer and InputActionMap as one bounded Runtime operation. An
application-owned ITouchInputFrameConsumer can pass its TouchInputFrameView
and current frame timestamp to process_frame(), then immediately consume the
returned action span in game logic.

The frame timestamp is mandatory and must share the monotonic clock domain
used by native PointerEvent::timestamp_ns values. The Runtime pipeline does
not read a platform clock. It processes empty touch frames too, so a stationary
pointer can produce LongPress without requiring a new MotionEvent or UIKit
touch callback. Gesture cancellation and surface-generation changes are
converted to cancelled action events; input validation or configured capacity
exhaustion returns a typed error without exposing partial action output.

The action span is owned by the pipeline and remains valid only until its next
process_frame() call or a successful rebinding operation. All constituent
objects are single-thread-affine. See
[ADR-0017](ADR-0017-RUNTIME-GESTURE-ACTION-INPUT-PIPELINE.md).

## Limitations

The Android NativeActivity and iOS UIKit shells feed their TouchInput objects,
and Runtime now provides a host-testable touch-to-gesture-to-action pipeline.
However, the packaged host shells still do not construct an application-owned
GameLoop, native frame source, or gameplay action consumer. Therefore no game
action currently reaches playable game logic in those shells. Virtual controls,
action contexts, device-specific bindings, sensor/haptics integration, and
physical-device input verification remain unimplemented. The API is experimental
and has no compatibility guarantee. See [ADR-0013](ADR-0013-NATIVE-TOUCH-ADAPTERS.md)
for the native adapter boundary and clock/coordinate decisions,
[ADR-0015](ADR-0015-PORTABLE-TOUCH-GESTURE-RECOGNITION.md) for gesture
semantics, and [ADR-0017](ADR-0017-RUNTIME-GESTURE-ACTION-INPUT-PIPELINE.md)
for Runtime composition.

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


## Gesture-to-gameplay Runtime adapter

`GestureActionGameLoopAdapter` connects a `TouchInput` instance and
`GestureActionInputPipeline` to an application-owned
`IGameActionFrameConsumer`. It runs in the Core input phase after
`MobileFrameDriver` supplies the absolute monotonic frame timestamp through
`IGameLoopCallbacks::begin_frame()`. The timestamp is required even when a
frame contains no pointer events, because LongPress recognition advances with
frame time.

The action consumer is invoked synchronously only for non-empty action
batches, before the delegated application's `process_input()` callback.
Action spans are non-owning and must be consumed during the callback. All
callbacks, the input state, pipeline, and consumer are single-thread-affine
and must outlive the adapter. The adapter clears frame events and deltas after
consumption, including pipeline or consumer failures, so a failed action
callback does not replay old touch transitions on a later frame.

The adapter is an integration component, not a gameplay implementation. A
game application supplies its own action consumer. The current Android/iOS
host shells do not yet instantiate one or connect a native frame source to
`MobileFrameDriver`.
