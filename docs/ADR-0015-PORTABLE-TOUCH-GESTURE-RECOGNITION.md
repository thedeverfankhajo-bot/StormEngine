# ADR-0015: Portable Touch Gesture Recognition

- Status: Accepted
- Date: 2026-11-03
- Scope: Platform-neutral recognition of common mobile pointer gestures

## Context

StormEngine already has a bounded, single-thread-affine TouchInput ingestion
contract. Android NativeActivity and the iOS host view translate native touch
callbacks into ordered PointerEvent batches, but the portable input module
does not interpret those transitions as gestures. Gesture interpretation must
not require Android, UIKit, Core, Scene, or rendering headers.

## Decision

Add Storm::Input::GestureRecognizer as a consumer-side component in the
existing Input library. The recognizer consumes an ordered span of pointer
events, a caller-supplied monotonic frame timestamp, and the current optional
surface generation. It does not own a clock, platform callback, surface, or
TouchInput object.

The initial supported gesture vocabulary is:

- Tap and DoubleTap
- LongPress
- single-pointer Drag Begin/Update/End
- Swipe with velocity in logical units per second
- two-pointer Pan and Pinch Begin/Update/End

A pinch event carries both stable pointer IDs and a scale relative to the
distance at pinch start. Two-pointer pan reports the centroid and centroid
delta. DoubleTap emits the second Tap as well as a DoubleTap event so callers
can choose whether to react to individual taps or the recognized pair.

All thresholds are configurable. Defaults are a movement threshold of 8
logical units, 250 ms maximum tap duration, 350 ms double-tap interval, 24
logical-unit double-tap distance, 500 ms long-press duration, and 700
logical-units-per-second minimum swipe velocity.

The recognizer reserves a configurable event-output capacity during creation
(default 256, maximum 4096) and stores active pointer tracks in a fixed array
bounded by TouchInput's supported pointer count. It validates finite
coordinates, monotonic event/frame timestamps, event timestamps not later than
the supplied frame time, pointer transition order, and surface-generation
identity. Surface changes cancel tracked gestures and invalidate pending
double-tap state. Invalid transitions or output-capacity exhaustion return a
typed error and reset partial recognizer state; the recognizer never silently
drops gesture output.

The output span is non-owning and remains valid only until the next update or
reset. The caller must consume or copy it synchronously on the recognizer's
owning thread.

## Consequences

Positive:

- Gesture policy is reusable across Android and iOS without platform-specific
  code or reverse dependencies.
- Timestamp and surface-lifetime validation remain explicit and testable.
- Output memory is bounded and reserved before frame processing.
- The recognizer can be used by a future ITouchInputFrameConsumer without
  changing the low-level TouchInput ingestion contract.

Limitations:

- The native host shells still do not instantiate an application-owned
  ITouchInputFrameConsumer or route recognized gestures into game actions.
- Virtual joysticks/buttons, action maps/contexts, rebinding, sensors, haptics,
  and device latency measurements are not included.
- Gesture thresholds are generic initial defaults; product-specific tuning
  and accessibility options require later design and device testing.
- The API is experimental and has no compatibility guarantee.

## Validation

The host test executable covers configuration validation, Tap/DoubleTap,
stationary LongPress, Drag/Swipe, two-pointer Pan/Pinch, surface-generation
cancellation, bounded-output exhaustion, and monotonic-frame rejection.
CI must compile and execute these cases under the existing GCC, Clang, Apple
Clang, and ASan/UBSan host configurations before the implementation is
reported as tested. Android/iOS cross-compilation checks the portable Input
library only; it does not verify native gesture delivery on a physical device.
