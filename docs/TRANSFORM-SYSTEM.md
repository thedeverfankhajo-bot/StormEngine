# Transform System (initial 3D slice)

## Scope and status

The Core TransformSystem is an initial, tested 3D transform slice. It is separate from rendering and accepts an optional `Storm::Math::Transform3D` component on any Scene entity.

Implemented contracts:

- Finite-value validation for translation, rotation, and scale.
- Quaternion identity, normalization, and axis-angle construction.
- 4x4 affine matrix construction and multiplication.
- Optional local Transform3D components; entities without one use identity.
- Parent-aware world matrix evaluation using the current Scene hierarchy.
- Stale/foreign entity rejection and malformed stored-transform diagnostics.
- Iterative ancestry evaluation, avoiding recursive call-stack growth for deep hierarchies.

This is not a full spatial, rendering, physics, or editor system. There is no transform cache, dirty propagation, scalar 1D transform integration, camera, renderer, or editor integration. The independent planar API is documented in [2D Transform System](TRANSFORM-2D-SYSTEM.md); it is not implemented by this 3D system.

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

`Matrix4::at(row, column)` checks both indices and throws `std::out_of_range` when either index is outside [0, 4).

## Usage

```cpp
#include "storm/core/transform_system.hpp"

const auto rotation = Storm::Math::Quaternion::from_axis_angle(
    Storm::Math::Vector3{0.0F, 0.0F, 1.0F},
    1.57079632679F);

if (!rotation) {
    // Handle rotation.error().
    return;
}

const Storm::Math::Transform3D local{
    .translation = {2.0F, 0.0F, 0.0F},
    .rotation = rotation.value(),
    .scale = {1.0F, 1.0F, 1.0F},
};

auto set_result = Storm::TransformSystem::set_local_transform(
    scene, player, local);
if (!set_result) {
    // Handle set_result.error().
    return;
}

auto world_result = Storm::TransformSystem::world_matrix(scene, player);
if (!world_result) {
    // Handle world_result.error().
    return;
}
```

Use `local_transform(scene, entity)` to read a local transform. It returns identity when the entity has no Transform3D component. `world_matrix(scene, entity)` returns a composed matrix for all ancestors that have a Transform3D component; hierarchy-only grouping entities are treated as identity.

## Performance and lifecycle

World matrices are evaluated on demand in O(depth) time and O(depth) temporary memory. The evaluation is iterative and validates each transform. This deliberately avoids cache invalidation rules while hierarchy mutation and component editing are still evolving. It is suitable as a correctness baseline, not a promise of high-throughput scene rendering. A cache or dirty-propagation system requires benchmarks and a documented invalidation contract before introduction.

Scene and component APIs remain single-thread-affine. Do not mutate the Scene concurrently with TransformSystem queries. Transform3D data is stored in the Scene's typed component storage and is removed automatically when its entity is destroyed.

## Dimensional boundaries

Transform3D is a general 3D affine transform usable by 3D content and as the spatial basis for future 2D systems (with explicit planar constraints to be designed separately). It does not implement 1D gameplay. StormEngine's 1D model must use scalar position/velocity/acceleration and one-axis collision rather than treating a 3D or 2D transform as a 1D world.

## Validation

`stormengine_math_tests` covers identity, normalization, invalid quaternion and axis-angle inputs, right-handed axis-angle rotation, TRS matrix construction, composition order, and matrix bounds. `stormengine_transform_system_tests` covers identity defaults, component replacement, handle validation, invalid values, parent composition, rotation/scale inheritance, grouping nodes, parent destruction, deep iterative ancestry, corrupt stored data, and Scene moves. Android and iOS CI compile the Math and Core libraries for their device architectures; no on-device transform behavior has been verified.
