# 2D Transform System (initial vertical slice)

## Scope and status

This first independent planar slice provides:

- `Vector2` and column-major `Matrix3` in `Storm::Math`.
- `Transform2D` with translation, rotation in radians, and non-uniform scale.
- `TransformSystem2D` for Scene component storage and parent-aware world matrices.
- A bounded Scene archive codec with stable ID `storm.math.transform2d`, schema version 1.

This is not a complete 2D engine. A 2D renderer, camera, sprite/texture pipeline, Physics2D, animation, tilemap, particles, audio integration, and editor workflow remain unimplemented.

## Coordinate and matrix convention

- +X points right and +Y points up; positive rotation is counter-clockwise.
- Matrices use column-major storage and column-vector multiplication.
- Local composition is Translation × Rotation × Scale.
- Child world composition is parentWorld × childLocal.
- Translation occupies column 2 (rows 0 and 1).
- Zero and negative scales are supported; NaN and infinity are rejected.
- Matrix multiplication uses double intermediates. An overflow becomes a non-finite result and is rejected by world-matrix evaluation.

These conventions do not depend on Transform3D. A Scene entity can own both component types, but each transform system evaluates only its own dimension.

## Usage

```cpp
#include "storm/core/transform_system_2d.hpp"

const Storm::Math::Transform2D local{
    .translation = {120.0F, 48.0F},
    .rotation_radians = 0.25F,
    .scale = {1.0F, 1.0F},
};
auto set = Storm::TransformSystem2D::set_local_transform(scene, player, local);
if (!set) {
    // Handle set.error().
}
auto world = Storm::TransformSystem2D::world_matrix(scene, player);
if (!world) {
    // Handle world.error().
}
```

Entities without Transform2D are identity grouping nodes. Scene handles are checked using the existing scene identity and generation rules.

## Evaluation and lifetime

World matrices are evaluated iteratively in O(depth) time and O(depth) temporary memory. There is no cache or dirty propagation. Scene access remains single-thread-affine; callers must not mutate hierarchy/components concurrently with a query. Component lifetime follows Scene entity-destruction rules.

## Serialization

The built-in codec ID is `storm.math.transform2d`, schema version 1. It stores five finite 32-bit values: translation X/Y, rotation radians, and scale X/Y. Deserialization validates the matrix representation before accepting the component. The enclosing Scene archive remains version 1.

## Validation boundary

Two host CTest executables register 8 transform and 3 serialization test cases. In Foundation CI run [41603833151](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41603833151), all 17 CTest executables passed under GCC, Clang, Apple Clang, and Clang ASan/UBSan; clang-tidy passed; Android arm64-v8a and iOS device arm64 targets compiled successfully. Android debug APK packaging and iOS unsigned app-bundle creation also passed. Cross-compilation and packaging are not runtime verification; no physical-device test is claimed.
