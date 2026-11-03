# Transform System and 3D Math

## Scope and status

The Core TransformSystem is an initial, tested 3D transform slice. It is separate from rendering and accepts an optional `Storm::Math::Transform3D` component on any Scene entity.

Implemented contracts:

- Finite-value validation for translation, rotation, and scale.
- Vector3 length, dot, cross, and checked normalization using double-precision intermediates.
- Quaternion identity, normalization, and axis-angle construction.
- 4x4 affine matrix construction, plus checked multiplication that rejects non-finite operands and results outside the float range.
- Right-handed perspective and orthographic camera projections with a zero-to-one depth range.
- Right-handed look-at view matrix construction.
- Checked homogeneous point transforms and translation-free direction transforms.
- Optional local Transform3D components; entities without one use identity.
- Parent-aware world matrix evaluation using the current Scene hierarchy.
- Stale/foreign entity rejection and malformed stored-transform diagnostics.
- Iterative ancestry evaluation, avoiding recursive call-stack growth for deep hierarchies.

This is not a full spatial, rendering, physics, or editor system. There is no transform cache, dirty propagation, scalar 1D transform integration, camera controller, renderer, GPU resource management, or editor integration. The independent planar API is documented in [2D Transform System](TRANSFORM-2D-SYSTEM.md); it is not implemented by this 3D system.

## Math convention

The API uses these conventions, which are part of the current development contract:

- Right-handed rotations with quaternion components ordered `x, y, z, w`.
- Column-major matrix storage.
- Column-vector multiplication.
- A local transform is composed as `Translation × Rotation × Scale`.
- A child world matrix is `parentWorld × childLocal`.
- The translation occupies matrix column 3 (rows 0 through 2).
- Quaternion inputs are normalized when converted to matrices. Zero and non-finite quaternions are rejected.
- Negative and zero scale are representable; non-finite values are rejected.

Camera math adds the following explicit contract:

- Camera view space is right-handed and looks down the negative Z axis; positive Y is up.
- `look_at_rh(eye, target, up)` builds a view matrix from a finite eye position, a distinct target, and a non-parallel up vector.
- `perspective_rh_zo` and `orthographic_rh_zo` map the near plane to normalized depth 0 and the far plane to 1. The projection is suitable for Vulkan and Metal clip-depth conventions.
- OpenGL ES backends must apply an explicit clip-depth conversion at the rendering-backend boundary; this math API does not silently switch conventions per device.
- Projection parameters and resulting coefficients are validated before narrowing to float. Non-finite values, invalid ranges, and coefficients outside the float range are rejected.
- `transform_point` applies homogeneous division and rejects points whose resulting `w` is zero or non-finite. `transform_direction` uses a homogeneous (w=0) and therefore ignores translation.

`Matrix4::at(row, column)` checks both indices and throws `std::out_of_range` when either index is outside [0, 4).

## Usage

```cpp
#include "storm/math/transform3d.hpp"

const auto projection = Storm::Math::Matrix4::perspective_rh_zo(
    1.04719755F, 16.0F / 9.0F, 0.1F, 1000.0F);
const auto view = Storm::Math::Matrix4::look_at_rh(
    {0.0F, 2.0F, 5.0F},
    {0.0F, 0.0F, 0.0F},
    {0.0F, 1.0F, 0.0F});

if (!projection || !view) {
    // Handle the typed error before submitting matrices to a renderer.
    return;
}

const Storm::Math::Matrix4 view_projection =
    projection.value() * view.value();
```

Use `local_transform(scene, entity)` to read a local transform. It returns identity when the entity has no Transform3D component. `world_matrix(scene, entity)` returns a composed matrix for all ancestors that have a Transform3D component; hierarchy-only grouping entities are treated as identity.

## Performance and lifecycle

World matrices are evaluated on demand in O(depth) time and O(depth) temporary memory. The evaluation is iterative and validates each transform. This deliberately avoids cache invalidation rules while hierarchy mutation and component editing are still evolving. It is suitable as a correctness baseline, not a promise of high-throughput scene rendering. A cache or dirty-propagation system requires benchmarks and a documented invalidation contract before introduction.

The camera constructors and `Matrix4::multiply_checked` use double-precision intermediate calculations, then reject values that cannot be represented by finite floats. The ordinary `operator*` remains an unchecked convenience operation and may produce a non-finite matrix on overflow; Core hierarchy composition uses the checked API and propagates its typed error. Vector normalization and cross products follow the same checked-narrowing rule. These APIs are platform-neutral and do not include graphics headers or device queries.

Scene and component APIs remain single-thread-affine. Do not mutate the Scene concurrently with TransformSystem queries. Transform3D data is stored in the Scene's typed component storage and is removed automatically when its entity is destroyed.

## Dimensional boundaries

Transform3D is a general 3D affine transform usable by 3D content and as the spatial basis for future 3D systems. It does not implement 1D gameplay. StormEngine's 1D model must use scalar position/velocity/acceleration and one-axis collision rather than treating a 3D or 2D transform as a 1D world.

## Validation

The `stormengine_math_tests` executable covers Vector3 arithmetic validation, quaternion operations, right-handed axis-angle rotation, TRS matrix construction, matrix composition, right-handed zero-to-one camera projections, look-at construction, homogeneous point/direction transformation, invalid numeric inputs, overflow rejection, and matrix bounds. `stormengine_transform_system_tests` covers identity defaults, component replacement, handle validation, invalid values, parent composition, rotation/scale inheritance, grouping nodes, parent destruction, deep iterative ancestry, corrupt stored data, Scene moves, and rejection of hierarchy scale overflow during checked matrix composition. Android and iOS CI compile the Math and Core libraries for their device architectures; no on-device transform or camera behavior has been verified.
