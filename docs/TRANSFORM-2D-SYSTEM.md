# 2D Transform System (initial vertical slice)

## Scope and status

This first independent planar slice provides:

- `Vector2` and column-major `Matrix3` in `Storm::Math`.
- `Transform2D` with translation, rotation in radians, and non-uniform scale.
- `TransformSystem2D` for Scene component storage and parent-aware world matrices.
- A bounded Scene archive codec with stable ID `storm.math.transform2d`, schema version 1.

This is not a complete 2D engine. A platform-neutral Camera2D math value is now available, but there is no Scene camera component, camera controller, viewport/surface binding, 2D renderer, sprite/texture pipeline, animation, tilemap, particles, audio integration, or editor workflow. Physics2D is an independent initial physics slice described in [PHYSICS-2D.md](PHYSICS-2D.md).

## Coordinate and matrix convention

- +X points right and +Y points up; positive rotation is counter-clockwise.
- Matrices use column-major storage and column-vector multiplication.
- Local composition is Translation × Rotation × Scale.
- Child world composition is parentWorld × childLocal.
- Translation occupies column 2 (rows 0 and 1).
- Zero and negative scales are supported; NaN and infinity are rejected.
- Checked Matrix3 multiplication accumulates in double and rejects non-finite inputs or coefficients outside the finite float range. The legacy unchecked operator remains source-compatible and marks out-of-range coefficients as infinity; TransformSystem2D uses the checked API.
- Camera2D uses +X-right/+Y-up world coordinates and positive counter-clockwise rotation. Its view matrix removes camera translation and rotation; its projection maps the visible camera rectangle to NDC [-1, +1]. `viewport_size` uses logical viewport units and `zoom` is a positive dimensionless magnification factor.

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

## Camera2D math

```cpp
#include "storm/math/transform2d.hpp"

const Storm::Math::Camera2D camera{
    .position = {120.0F, 48.0F},
    .rotation_radians = 0.0F,
    .viewport_size = {800.0F, 600.0F}, // logical viewport units
    .zoom = 1.0F,
};
const auto view_projection = camera.view_projection();
if (!view_projection) {
    // Handle view_projection.error().
}
const auto player_ndc = camera.world_to_ndc({140.0F, 60.0F});
if (!player_ndc) {
    // Handle the typed validation/range error.
}
```

Camera2D is a platform-neutral math type, not a Scene component or renderer. The application must update the viewport size when its logical surface changes. It uses positive Y-up coordinates and NDC [-1, +1] on both axes; any backend-specific framebuffer/viewport Y conversion belongs in the future renderer boundary.

## Evaluation and lifetime

World matrices are evaluated iteratively in O(depth) time and O(depth) temporary memory. There is no cache or dirty propagation. Scene access remains single-thread-affine; callers must not mutate hierarchy/components concurrently with a query. Component lifetime follows Scene entity-destruction rules.

## Serialization

The built-in codec ID is `storm.math.transform2d`, schema version 1. It stores five finite 32-bit values: translation X/Y, rotation radians, and scale X/Y. Deserialization validates the matrix representation before accepting the component. The enclosing Scene archive remains version 1.

## Validation boundary

Two host CTest executables register 11 Transform2D/Camera2D test cases and 3 serialization test cases. In Foundation CI run [41603833151](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41603833151), all 17 CTest executables passed under GCC, Clang, Apple Clang, and Clang ASan/UBSan; clang-tidy passed; Android arm64-v8a and iOS device arm64 targets compiled successfully. Android debug APK packaging and iOS unsigned app-bundle creation also passed. Cross-compilation and packaging are not runtime verification; no physical-device test is claimed. Camera2D math is host-testable and does not submit draw calls or allocate GPU resources.
