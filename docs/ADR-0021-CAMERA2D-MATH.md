# ADR-0021: Platform-Neutral Camera2D Math

- Status: Accepted for the initial 2D camera math slice
- Date: 2026-11-03

## Context

StormEngine has independent planar Transform2D and Matrix3 math, but no camera math. A 2D renderer will need a deterministic transform from world coordinates into normalized device coordinates. The math must remain independent of rendering APIs, mobile SDKs, and surface handles.

## Decision

- Add a plain `Camera2D` value type to the existing `Storm::Math` target; do not add a Scene camera component or renderer dependency.
- Define camera world coordinates as +X right and +Y up, with positive rotation counter-clockwise.
- Interpret `viewport_size` as finite positive logical viewport dimensions and `zoom` as a positive dimensionless magnification.
- Construct a right-handed planar view matrix from camera position and rotation.
- Construct an orthographic projection that maps the visible rectangle to NDC [-1, +1] on X and Y.
- Provide checked `Matrix3::multiply_checked` and homogeneous `Matrix3::transform_point` operations for safe composition and world-to-NDC conversion.
- Keep the legacy `Matrix3::operator*` for compatibility; runtime hierarchy composition and camera composition use the checked API.
- Leave backend-specific framebuffer Y conversion, viewport/scissor state, and surface lifecycle handling to a future renderer.

## Validation

- Verify camera center maps to NDC origin.
- Verify viewport edges map to NDC corners.
- Verify camera rotation affects view orientation in the documented direction.
- Reject invalid zoom, viewport sizes, non-finite inputs, homogeneous divide failures, and unrepresentable matrix coefficients.
- Exercise checked Matrix3 overflow and Scene hierarchy composition.

## Consequences

- Camera math is now independently usable and testable without GPU or OS dependencies.
- This does not constitute a Scene camera component, camera controller, renderer, GPU integration, or device verification.
- The logical viewport size must be refreshed by the application when the surface changes.
