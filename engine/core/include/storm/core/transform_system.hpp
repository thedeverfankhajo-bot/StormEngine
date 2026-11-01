#pragma once

#include "storm/core/scene.hpp"
#include "storm/math/transform3d.hpp"

namespace Storm {

// Computes hierarchy-aware 3D transforms from optional Transform3D components.
// Missing components are treated as identity; world matrices are evaluated on demand.
class TransformSystem final {
public:
    [[nodiscard]] static Result<void> set_local_transform(
        Scene& scene,
        EntityHandle entity,
        const Math::Transform3D& transform);

    // Returns identity when the entity has no Transform3D component.
    [[nodiscard]] static Result<Math::Transform3D> local_transform(
        const Scene& scene,
        EntityHandle entity);

    // Uses parent_world * local composition. No world-transform cache is kept.
    [[nodiscard]] static Result<Math::Matrix4> world_matrix(
        const Scene& scene,
        EntityHandle entity);
};

} // namespace Storm
