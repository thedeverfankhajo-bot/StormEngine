# ADR-0019: 3D Camera Projection and View-Space Convention

- Status: Accepted for the platform-neutral 3D Math foundation
- Date: 2026-11-03

## Context

StormEngine already defines right-handed quaternion rotations, column-major matrices, and column-vector multiplication. A useful 3D runtime also needs camera view and projection matrices. The convention must be independent of graphics APIs and deterministic across mobile targets. The initial Android and iOS graphics candidates include Vulkan and Metal, whose clip-space depth range is zero-to-one; a future OpenGL ES compatibility backend has a different clip-depth convention.

## Decision

- Camera view space is right-handed, looks down the negative Z axis, and uses positive Y as up.
- The view constructor is `look_at_rh(eye, target, up)`; coincident eye/target and a parallel up vector are rejected.
- Perspective and orthographic projections use a normalized device depth interval of [0, 1], with near mapping to 0 and far mapping to 1.
- Perspective takes a vertical field of view in radians, an explicit positive aspect ratio, and positive ordered near/far distances.
- Projection builders calculate in double precision, verify float representability, and return typed errors rather than silently storing infinities.
- Point transforms apply homogeneous division; direction transforms use (w=0) and ignore translation.
- OpenGL ES depth conversion, if that backend is selected, belongs in the graphics-backend boundary. The platform-neutral Math contract does not vary based on the active device.

## Alternatives considered

- Negative-one-to-one clip depth: rejected as the common engine convention because it would require a conversion for Vulkan and Metal.
- Per-backend projection conventions in Math: rejected because game code and serialized camera settings should not depend on the renderer selected at runtime.
- Implicit aspect-ratio derivation from a surface: deferred to a future Camera component, where viewport lifecycle and aspect updates can be tested explicitly.

## Consequences

- Math remains independent of graphics SDK headers and mobile platform APIs.
- Vulkan and Metal backends can consume the zero-to-one projection directly, subject to their viewport and clip-space configuration.
- An OpenGL ES backend must apply an explicit depth conversion at the backend boundary.
- These functions do not constitute a renderer, camera component, surface integration, frustum culling, or device-runtime validation.
- Camera matrix behavior is host-testable without a GPU.
