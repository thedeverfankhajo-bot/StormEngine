# ADR-0010: Portable Touch Input Boundary

- Status: Accepted
- Date: 2026-11-02

## Context

Touch input is shared by the 1D, 2D, and 3D game models. The repository has
platform lifecycle and surface metadata contracts, but no native pointer-input
abstraction. Direct OS types in Core would couple portable game logic to
Android/UIKit, while unsynchronized native callbacks would make threading and
lifecycle cancellation ambiguous.

## Decision

Add a standalone C++20 `Storm::Input` target depending only on
`Storm::Foundation`. It accepts platform-neutral pointer events with logical
coordinates, monotonic timestamps supplied by the caller, and a surface
generation. It supports Down, Move, Up, Cancel, multiple active IDs, per-frame
deltas, bounded buffers, and explicit cancellation on focus/surface loss.

The input object is single-thread-affine. Future Android/iOS adapters must
serialize native callbacks onto its owning thread. This target owns no native
handles, clock, surface, renderer resource, or platform permission.

## Consequences

- Core and Physics remain independent of platform input APIs.
- Stale callbacks from recreated surfaces are rejectable.
- Capacity exhaustion is explicit instead of silently dropping events.
- Gesture recognition, virtual controls, action mapping, sensors, haptics,
  native adapters, and device verification remain future work.
- The current API is experimental until native integration and compatibility
  testing establish a stable contract.
