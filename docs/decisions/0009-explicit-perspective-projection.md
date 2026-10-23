# ADR 0009: Explicit right-handed perspective projection

- **Status:** Accepted for the foundational math API
- **Date:** 2026-10-23
- **Scope:** CPU-side projection mathematics; not a renderer-backend ABI decision

## Context

The engine now has dimension-aware transforms, a shared Matrix4 value, and a quaternion rotation primitive. A 3D renderer will need a perspective projection, but graphics APIs do not all use the same clip-space depth range or framebuffer conventions. An implicit "one true" projection risks subtle near/far clipping errors when a renderer backend is selected later.

## Decision

Provide `storm::math::Projection::perspective_rh` in `include/storm/math/projection.hpp`. The function returns `std::optional<Matrix4>` and requires the caller to choose `ClipDepthRange::ZeroToOne` or `ClipDepthRange::NegativeOneToOne` explicitly.

The contract is:

1. The view is right-handed, with the camera looking along local `-Z`; matrices multiply column vectors and use the existing `Matrix4` column-major storage.
2. `vertical_fov_radians` is the full vertical field of view in radians and must be strictly between 0 and pi.
3. `aspect_ratio` is viewport width divided by height and must be finite and positive.
4. The caller supplies finite clip distances with `0 < near_plane < far_plane`.
5. `ZeroToOne` maps the camera-space near plane to normalized depth 0 and the far plane to 1. `NegativeOneToOne` maps the near plane to -1 and the far plane to 1.
6. X/Y are not flipped. Backend integrations must handle any framebuffer-origin, clip-Y, viewport or shader conventions explicitly rather than relying on this generic math function.
7. Invalid enums, non-finite values, invalid plane ordering, and matrix overflow return no result instead of providing a malformed matrix.

For a point `p` in view space, use `clip = projection * p`; divide XYZ by W only after applying the projection. The matrix uses `Wclip = -Zview`, so points in front of the camera have positive homogeneous W.

## Deliberate non-goals

This first version does not add reversed-Z, an infinite far plane, temporal jitter, lens shift, orthographic 3D projection, left-handed views, or a GPU resource/backend contract. The selected depth range is explicit because, for example, Vulkan clips depth using `0 <= Zc <= Wc`; backend conversion belongs in renderer integration. See the [official Vulkan depth guide](https://docs.vulkan.org/guide/latest/depth.html).

## Validation

Unit tests cover both depth ranges, near/far mapping, frustum edge mapping, invalid FOV/aspect/clip values, invalid enum values, and arithmetic overflow. The installed-package consumer also compiles against the public header and checks near/far mapping after relocation of the install tree.

## Consequences

- Camera/projection math can be validated headlessly before a GPU renderer exists.
- Renderer backends must select their depth range deliberately.
- No existing Transform3D fields, SceneCodec versions, or persisted scene records are changed.
