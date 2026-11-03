# ADR-0013: Native Touch Input Adapters

- Status: Accepted
- Date: 2026-11-03
- Scope: Android NativeActivity and iOS host-view touch ingestion

## Context

Storm::Input provides a bounded, platform-neutral, single-thread-affine
TouchInput state machine. The project previously had no native touch callback
path, so Android/iOS host surfaces could not submit real pointer transitions.
Core and Runtime must remain independent of Android NDK and UIKit types.

## Decision

Keep TouchInput platform-neutral and add thin native adapters at the application
host boundary. Neither adapter introduces platform headers into Core,
Foundation, or the portable Input library.

### Android

The NativeActivity host registers android_app::onInputEvent and maps NDK
MotionEvent Down/PointerDown, Move, PointerUp/Up, and Cancel actions to
PointerEvent batches. It uses the stable MotionEvent pointer ID rather than the
transient pointer index. Unknown, finger, stylus, and eraser tools are accepted;
mouse, palm, and other non-touch tools are ignored.

MotionEvent X/Y values are window-relative pixels. They are divided by the
active SurfaceDescriptor pixels_per_logical_unit value to produce logical
coordinates. The event timestamp is passed through in the
java.lang.System.nanoTime nanosecond time base. A surface geometry or density
change cancels current pointers before subsequent events use the updated
coordinate scale. Focus loss and surface detachment also cancel all pointers.

### iOS

The iOS host view enables multiple touches and maps UIKit
touchesBegan/Moved/Ended/Cancelled callbacks to the same PointerPhase contract.
Coordinates use locationInView: in the host view's logical point space. A
UITouch object's identity is used as the pointer ID for its active lifetime.
The enclosing UIEvent timestamp is converted from seconds since system startup
to nanoseconds; this gives all pointers in one callback batch the same event
timestamp. Touches are cancelled when UIKit reports touchesCancelled, when the
scene leaves ForegroundActive, or when the view surface detaches.

### Shared contract

Both adapters:
- Attach TouchInput to the SurfaceLifecycle generation and reject stale surface
  events.
- Serialize callback processing on their owning native thread.
- Use the bounded TouchInput::process_batch API. If any event in a batch fails,
  partial frame events are discarded and active pointers are cancelled.
- Keep the host-owned TouchInput instance separate from Core/gameplay. A
  later vertical slice must define a frame-level consumer that transfers
  completed input state/events into Runtime/Core without exposing native SDKs.

## Consequences and validation boundary

The native adapters can be compiled and linked into the Android NativeActivity
shared library and the iOS host app while preserving the platform-independent
input API. CI cross-compilation verifies SDK declarations, linkage, and target
compiler compatibility only. It does not prove callback delivery on a physical
device; Android and iOS device input tests remain required.

The implementation does not add gesture recognition, input actions/mapping,
rebinding, virtual controls, sensors, haptics, or a Core gameplay input
consumer. These remain separate work items rather than implied capabilities.
