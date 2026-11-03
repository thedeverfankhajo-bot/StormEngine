# ADR-0017: Runtime Gesture-to-Action Input Pipeline

- Status: Accepted
- Date: 2026-11-03
- Scope: Connect frame-scoped touch input to stable game actions without platform coupling

## Context

StormEngine has separate, host-tested implementations for native touch ingestion,
gesture recognition, and gesture-to-action mapping. The Runtime layer already
offers a frame-scoped TouchInputGameLoopAdapter, but applications otherwise
have to assemble GestureRecognizer and InputActionMap themselves. That leaves
a common integration path implicit and makes it easier to mishandle timestamps,
surface recreation, or non-event frames used by time-based gestures.

## Decision

Add GestureActionInputPipeline to Storm::Runtime. It composes the existing
GestureRecognizer and InputActionMap and consumes the non-owning
TouchInputFrameView produced by TouchInputGameLoopAdapter.

The application calls process_frame() from its ITouchInputFrameConsumer and
supplies the current monotonic frame timestamp. The timestamp must use the
same monotonic clock domain as PointerEvent::timestamp_ns. The pipeline does
not read a clock or choose a native frame source; Android and iOS hosts retain
ownership of those platform decisions.

The pipeline processes an entire touch frame in order:

1. Validate and recognize pointer transitions and time-based gestures.
2. Map recognized gestures to caller-defined InputActionId values.
3. Return a non-owning span of InputActionEvent values for immediate gameplay
   consumption.

A frame with no pointer transitions is still processed. This is required for
LongPress recognition while a pointer remains stationary. Binding replacement
is exposed through an atomic InputActionMap operation. The Runtime layer
continues to avoid platform SDK headers and keeps Core independent of Input.

## Lifetime, threading, and failure contract

The pipeline is single-thread-affine, like TouchInput, GestureRecognizer, and
InputActionMap. Its input view is borrowed only during process_frame(). The
returned action span belongs to the pipeline and is invalidated by the next
process_frame() call or a successful binding replacement.

All capacities remain bounded by the existing recognizer and action-map
configuration. process_frame() performs no new allocation within configured
limits. Invalid/non-monotonic timestamps, stale surface events, invalid gesture
payloads, or exhausted output budgets are returned as typed errors; partial
action output is not exposed. The host decides whether an input-phase error
aborts the current game-loop frame.

## Consequences

Positive:

- Applications have one standard Runtime path from touch frames to game actions.
- Time-based gestures can advance on frames with no new native touch event.
- Timestamp origin remains explicit and consistent with native event timestamps.
- The implementation reuses existing bounds, validation, and action lifecycle
  semantics instead of duplicating them.

Limitations:

- The Android and iOS sample host shells do not yet construct an application
  GameLoop, native frame source, or gameplay action consumer.
- This pipeline does not add action contexts, virtual controls, sensors,
  haptics, or physical-device verification.
- A successful host test or mobile cross-build is not device verification.

## Validation

The dedicated Runtime test target covers touch-to-tap action mapping,
LongPress on an eventless frame, cancellation propagation, surface-generation
change cancellation, non-monotonic frame timestamps, and action-output
exhaustion. CI must pass the target under host compilers and sanitizers and
compile the Runtime target for Android arm64-v8a and iOS device arm64.
