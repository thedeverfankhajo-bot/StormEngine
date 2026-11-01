# Core Event System

## Scope

`Storm::EventDispatcher` is a synchronous, type-safe publish/dispatch mechanism in the platform-neutral Core layer. Event types are identified by their exact C++ type. The dispatcher does not require an inheritance hierarchy, reflection, a platform SDK, or an external dependency.

The dispatcher is separate from `Storm::Platform::LifecycleEventQueue`. That queue is a bounded multi-producer/single-consumer transport for normalized operating-system lifecycle events. `EventDispatcher` invokes application/game handlers synchronously on the calling thread; it is not a cross-thread queue.

## Basic usage

```cpp
struct ScoreChanged final {
    int score{0};
};

Storm::EventDispatcher events;

auto subscription = events.subscribe<ScoreChanged>(
    [](const ScoreChanged& event) {
        // Update game-facing state.
    });

if (!subscription) {
    // Handle a subscription setup error.
}

const auto result = events.dispatch(ScoreChanged{.score = 120});
if (!result) {
    // Handle the first failing handler.
}
```

A handler may return either `void` or `Storm::Result<void>`. A failed result stops the current dispatch and is returned with event-dispatch context. Exceptions are not caught and propagate to the caller.

## Subscription lifetime and ordering

- `subscribe<Event>(handler)` returns a move-only RAII `Subscription`.
- The subscription disconnects when explicitly disconnected or destroyed.
- Moving a subscription transfers the connection; the moved-from handle becomes disconnected.
- Handlers run in subscription order.
- Dispatch snapshots the listener list when it starts. A listener added during dispatch begins receiving events on the next dispatch.
- A listener disconnected during dispatch is skipped immediately if it has not run yet. `clear()` has the same immediate effect on the pending listeners.
- A handler failure stops later handlers. Side effects from handlers that already ran are not rolled back.
- Dispatch without listeners succeeds and returns zero completed handlers.

The same dispatcher may be used reentrantly from a handler. Recursive event production is the caller's responsibility and can recurse without a built-in depth limit.

## Threading and ownership

The dispatcher is not internally synchronized. Subscribe, disconnect, clear, and dispatch operations must be confined to one owning thread. Do not call it concurrently from platform callbacks or worker threads.

Native lifecycle messages currently have their own bounded platform queue. A future general cross-thread game-event queue must define capacity, overflow, ordering, ownership, and shutdown semantics independently; this dispatcher does not pretend to provide those guarantees.

Handlers receive a const reference to the event, valid only for the duration of the synchronous call. Handlers must not retain that reference. The dispatcher owns callable objects through the subscription slot; captured object lifetimes remain the application's responsibility.

## Validation boundary

The dedicated host test executable contains cases for dispatch order and exact type matching, no-listener behavior, RAII and move semantics, disconnection and registration during dispatch, clearing during dispatch, failure propagation, empty callback rejection, dispatcher destruction, and idempotent disconnect. The CI run for the commit that adds these cases is the evidence required before marking them as passed.

These tests validate the platform-neutral dispatcher only. They do not demonstrate OS event integration or physical-device runtime behavior.
