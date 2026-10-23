# ADR 0002: Coordinate system and matrix conventions

- Status: Accepted for the foundational math API
- Date: 2026-10-23
- Scope: engine world-space math; not a renderer-backend or serialized-file ABI decision

## Context

Transforms already store radians for rotation, and the engine targets one 1D/2D/3D scene model across desktop and Android. A matrix convention must be explicit before camera, rendering, physics, or transform composition code is built on top of it. The convention should be internally consistent and should not pretend that every graphics API uses the same clip space or GPU memory layout.

## Decision

1. The initial 3D world convention is right-handed: +X points right, +Y points up, and +Z points toward the viewer. A camera at its local origin looks along local -Z.
2. All rotation angles exposed by the math and scene APIs are radians. Positive axis rotations follow the right-hand rule.
3. Matrix mathematics uses column vectors. A composed transform `A * B * p` applies `B` first and then `A`; a typical future rendering chain is `projection * view * model * position`.
4. `Matrix4` stores 16 floats in column-major order. Mathematical element access is `matrix(row, column)`, and the packed index is `column * 4 + row`.
5. Translation occupies column 3. The X/Y/Z rotation constructors use the standard right-handed forms. `transform_point` applies homogeneous coordinates and divides by W; it rejects non-finite data and near-zero W. `transform_direction` ignores translation and perspective division and does not normalize the result.
6. `Transform::to_matrix()` embeds 1D on the X axis (Y/Z remain unchanged), 2D on the XY plane (rotation around +Z, with Z unchanged), and 3D into full world space. For 3D Euler components, the applied order is X, then Y, then Z; with column vectors the composed matrix is `T * Rz * Ry * Rx * S`, so a point is scaled first, rotated, then translated. Matrix conversion returns no result if source values or the computed matrix are non-finite.
7. No world-distance unit (metres, centimetres, or otherwise) is imposed yet. Backend upload layout and Y-axis conversion remain separate decisions. The explicit CPU perspective-projection contract and its two supported depth ranges are documented in [ADR 0009](0009-explicit-perspective-projection.md).
8. This is a math API convention, not a serialized format or GPU ABI guarantee. Callers must not serialize the raw matrix array as a stable file format or upload it to a graphics API without an explicit backend-side layout/convention conversion.
9. `Quaternion` stores components as `(w, x, y, z)` and represents an active right-handed rotation. `from_axis_angle` accepts radians and rejects non-finite angles, invalid epsilon and axes too close to zero. Rotation and matrix conversion normalize the input first and reject non-finite/near-zero quaternions or numerically invalid results. Hamilton composition obeys the same column-vector order: `a * b` rotates by `b` before `a`. Quaternion values are currently a standalone math API; they do not change the Euler fields in `Transform3D` or the versioned scene file format.

The distinction around projection is intentional: for example, Vulkan's clip-volume depth constraint is expressed as `0 <= Zc <= Wc`, with normalized device depth in [0, 1]. Projection construction belongs with a renderer/backend decision rather than being silently implied by this generic matrix type. See the [official Vulkan depth guide](https://docs.vulkan.org/guide/latest/depth.html).

## Consequences

- Tests can verify rotation signs, matrix composition, packed layout, point-versus-direction behavior, and invalid homogeneous coordinates without a GPU.
- A future camera and render RHI can consume a consistent internal convention and apply an explicit conversion when a backend needs a different convention.
- The current Transform structs continue to store local values only; this ADR does not add scene hierarchy, world transforms, projection matrices, physics units, or a renderer.
- Any future persisted matrix representation must declare its own format version, precision, and byte order rather than copying C++ object memory.
