# ADR-0007: iOS UIView Surface Lifecycle Mapping

- Status: Accepted for the metadata-only iOS host integration
- Date: 2026-11-01
- Scope: iOS host view geometry and SurfaceLifecycle

## Context

StormEngine already has a platform-neutral, generation-checked SurfaceLifecycle contract and an Android NativeActivity adapter that reports native-window pixel dimensions and display scale. The iOS host app did not yet map UIKit view geometry into that contract. A renderer needs a validated, current surface description, but this step must not imply that GPU drawable creation or rendering already exists.

## Decision

- The iOS host root view controller owns one Storm::Platform::SurfaceLifecycle instance for its view lifetime.
- UIView.bounds is treated as logical points. UIView.contentScaleFactor is passed as pixels per logical unit.
- A shared C++ helper converts logical width/height and scale to a checked SurfaceExtent, rounding to the nearest integer pixel. It rejects non-finite/non-positive values, empty pixel extents, and values that cannot fit the 32-bit extent representation.
- The view attaches on its first valid layout, resizes the active generation as bounds or scale changes, and detaches when geometry becomes unavailable or the view disappears.
- The host uses a per-instance opaque ID, not a borrowed UIView, CALayer, or pointer value. SurfaceLifecycle retains metadata only.
- The host status label displays the current pixel extent and scale as diagnostic information.

## Consequences

Positive:
- iOS and Android now report surface dimensions through the same platform-neutral contract.
- Point-to-pixel conversion and overflow behavior are host-testable without UIKit.
- UIKit types remain confined to the iOS host integration layer.

Limitations:
- This does not create a CAMetalLayer drawable, allocate GPU resources, manage a swapchain, or implement a renderer.
- The iOS surface metadata is not yet consumed by MobileFrameDriver or a native frame source.
- No physical-device validation has been performed for rotation, scale changes, scene disconnection, or view recreation.

## Validation

- Host C++ tests cover standard scale conversion, rounding, invalid metrics, empty pixel extents, and overflow.
- The iOS Objective-C++ host is compiled and packaged by the iOS Device SDK job in Foundation CI.
- Physical-device behavior remains unverified until tested on named iPhone/iPad models and OS versions.
