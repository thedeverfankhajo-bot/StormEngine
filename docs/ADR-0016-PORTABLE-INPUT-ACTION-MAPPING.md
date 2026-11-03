# ADR-0016: Portable Input Action Mapping

- Status: Accepted
- Date: 2026-11-03
- Scope: Mapping recognized gestures to stable game-defined input actions

## Context

StormEngine now recognizes common single- and multi-pointer gestures in the
platform-neutral Input library. Games still need a stable way to bind those
gestures to gameplay actions without introducing platform headers or coupling
Input to Core, Scene, Runtime, or Editor.

## Decision

Add InputActionMap to the existing Storm::Input library. Bindings map a
GestureType to a caller-owned, non-zero 32-bit InputActionId. Multiple distinct
action IDs may be bound to one gesture. Duplicate gesture/action pairs are
rejected. Binding order is stable and determines output order when several
actions share one gesture.

Action phases are derived from gesture lifecycle:

- DragBegin, PanBegin, PinchBegin -> Started
- one-shot gestures and update transitions -> Performed
- non-cancelled DragEnd, PanEnd, PinchEnd -> Completed
- cancelled endings and explicit Cancelled gestures -> Cancelled

Each InputActionEvent retains the source GestureEvent, including pointer IDs,
timestamp, coordinates, deltas, scale, velocity, and cancellation metadata.

Bindings can be replaced at runtime. Replacement validates the full candidate
set, constructs it separately, and swaps it only after successful allocation,
so invalid input or allocation failure preserves the previous mapping. An
empty replacement intentionally clears all bindings.

The map is single-thread-affine. It reserves output storage during creation;
update() validates the complete gesture batch before mapping, checks finite
payload values and positive scale, rejects unknown gesture types and
non-monotonic timestamps across calls, and emits no partial output on an error.

Default capacities are 128 bindings, 4096 gestures per update, and 256 action
events per update. Hard limits are 1024 bindings, 4096 gestures per update,
and 8192 action events per update. update() performs no allocation within
these limits. Binding replacement is an explicit configuration operation and
may allocate.

## Consequences

Positive:

- Game code can use stable action IDs instead of depending directly on native
  gesture APIs or pointer callback details.
- A gesture can intentionally trigger more than one gameplay action.
- Rebinding is atomic and does not leave a partially applied binding table.
- Action output and per-update work have explicit upper bounds.
- The Input library remains independent of Core, Runtime, Editor, UIKit, and
  Android NDK headers.

Limitations:

- This is a gesture-to-action mapping layer, not yet a full Input Context
  stack or priority/focus system.
- Device-specific physical buttons, sensors, configurable dead zones,
  virtual joysticks/buttons, and action persistence are not included.
- The Android/iOS native hosts do not yet instantiate an application-owned
  consumer to route action events into game logic.
- The API remains experimental and has no compatibility guarantee.

## Validation

The host test executable covers configuration and binding validation,
gesture lifecycle phase conversion, multiple actions per gesture and stable
ordering, cancellation propagation, atomic rebinding, output-capacity
exhaustion, invalid numeric data, stale/non-monotonic timestamps, and gesture
batch limits. Its integration case drives a real TouchInput Down/Up sequence
through GestureRecognizer and then into InputActionMap, asserting the final
action ID, phase, pointer identity, and timestamp. CI must pass the new test target under GCC, Clang, Apple Clang,
and ASan/UBSan before the capability is reported as tested. Android/iOS target
builds prove portable Input library compilation only, not physical-device
gameplay integration.
