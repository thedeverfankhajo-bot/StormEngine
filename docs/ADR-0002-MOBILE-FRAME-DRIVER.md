# ADR-0002: Add a platform-aware mobile frame driver outside Core

- Date: 2026-11-01
- Status: Accepted
- Scope: Runtime frame scheduling and mobile lifecycle/surface coordination

## Context

`Storm::GameLoop` accepts a caller-supplied elapsed duration and remains independent of native APIs. `Storm::Platform::LifecycleStateMachine` decides whether a presentation stream is active, while `Storm::Platform::SurfaceLifecycle` validates native surface identity and generation. These separate contracts were not yet joined by a runtime component.

A simple comparison of the current lifecycle enum is insufficient: an app may move from active to background and back between two frame calls. If the runtime sees only the final active state, it could incorrectly include background time in the next simulation delta.

## Decision

Add `Storm::Runtime::MobileFrameDriver` in a separate `storm_runtime` target that depends on both `Storm::Core` and `Storm::Platform`. Do not add a Platform dependency to Core.

The driver:

- Runs Core callbacks only while lifecycle is `ForegroundActive` and a validated surface is attached.
- Accepts a caller-supplied `std::chrono::steady_clock::time_point`; it does not own or sample a clock.
- Uses a monotonically increasing lifecycle revision and the surface generation to detect transitions/recreation between frame calls.
- Uses zero elapsed time on the first frame after activation, resume, surface loss/recreation, or explicit timing reset.
- Leaves ordinary resize events on the same generation alone.
- Returns typed errors for a regressing timestamp or elapsed values that cannot fit in nanoseconds.
- Remains single-thread-affine and propagates Core callback failures.

## Consequences

- Core remains platform-neutral and independently host-testable.
- Runtime provides one place to enforce the mobile lifecycle/surface gate and fixed-step clock policy.
- Native Android/iOS frame sources still need to call the driver; this ADR does not claim Choreographer, CADisplayLink, rendering, app packaging, or device integration.
- A future multithreaded runtime must hand lifecycle and surface updates to the engine thread using a bounded, explicit queue. It must not concurrently call the current unsynchronized contracts.

## Validation required

- Unit tests for inactive/no-surface gating, fixed-step delta, lifecycle revision, surface generation, resize, timestamp regression, and callback failure.
- Host CI builds and tests.
- Android NDK and iOS Device SDK cross-compilation of the Runtime target.
