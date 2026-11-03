#include "storm/core/transform_system.hpp"

#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace Storm {
namespace {

[[nodiscard]] Error invalid_entity_error() {
    return Error::make(
        ErrorCode::InvalidState,
        "The EntityHandle is stale, invalid, or belongs to another Scene");
}

[[nodiscard]] Error resource_error(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

} // namespace

Result<void> TransformSystem::set_local_transform(
    Scene& scene,
    EntityHandle entity,
    const Math::Transform3D& transform) {
    if (!scene.is_alive(entity)) {
        return Result<void>::failure(invalid_entity_error());
    }
    if (!transform.is_valid()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A local Transform3D must contain finite values and a non-zero rotation"));
    }

    if (Math::Transform3D* existing =
            scene.try_get_component<Math::Transform3D>(entity)) {
        *existing = transform;
        return Result<void>::success();
    }

    try {
        const auto added =
            scene.add_component<Math::Transform3D>(entity, transform);
        if (!added) {
            return Result<void>::failure(added.error());
        }
    } catch (const std::bad_alloc&) {
        return Result<void>::failure(
            resource_error("The Scene could not allocate Transform3D component storage"));
    } catch (const std::length_error&) {
        return Result<void>::failure(
            resource_error("The Scene reached a Transform3D storage capacity limit"));
    }

    return Result<void>::success();
}

Result<Math::Transform3D> TransformSystem::local_transform(
    const Scene& scene,
    EntityHandle entity) {
    if (!scene.is_alive(entity)) {
        return Result<Math::Transform3D>::failure(invalid_entity_error());
    }

    const Math::Transform3D* transform =
        scene.try_get_component<Math::Transform3D>(entity);
    if (transform == nullptr) {
        return Result<Math::Transform3D>::success(Math::Transform3D{});
    }
    if (!transform->is_valid()) {
        return Result<Math::Transform3D>::failure(Error::make(
            ErrorCode::InvalidState,
            "The entity contains an invalid Transform3D component"));
    }

    return Result<Math::Transform3D>::success(*transform);
}

Result<Math::Matrix4> TransformSystem::world_matrix(
    const Scene& scene,
    EntityHandle entity) {
    if (!scene.is_alive(entity)) {
        return Result<Math::Matrix4>::failure(invalid_entity_error());
    }

    std::vector<EntityHandle> ancestry;
    try {
        const std::size_t maximum_depth = scene.entity_count();
        EntityHandle current = entity;
        for (;;) {
            if (ancestry.size() >= maximum_depth) {
                return Result<Math::Matrix4>::failure(Error::make(
                    ErrorCode::InvalidState,
                    "The Scene hierarchy contains a cycle or exceeds its entity count"));
            }

            ancestry.push_back(current);
            const auto parent = scene.parent_of(current);
            if (!parent) {
                return Result<Math::Matrix4>::failure(parent.error());
            }
            if (!parent.value().has_value()) {
                break;
            }
            current = *parent.value();
        }
    } catch (const std::bad_alloc&) {
        return Result<Math::Matrix4>::failure(
            resource_error("Unable to allocate the Transform3D ancestry chain"));
    } catch (const std::length_error&) {
        return Result<Math::Matrix4>::failure(
            resource_error("The Transform3D ancestry chain exceeded container limits"));
    }

    Math::Matrix4 world = Math::Matrix4::identity();
    for (auto iterator = ancestry.rbegin(); iterator != ancestry.rend(); ++iterator) {
        const Math::Transform3D* local =
            scene.try_get_component<Math::Transform3D>(*iterator);
        if (local == nullptr) {
            continue;
        }

        const auto local_matrix = local->to_matrix();
        if (!local_matrix) {
            return Result<Math::Matrix4>::failure(local_matrix.error());
        }

        const auto composed = Math::Matrix4::multiply_checked(
            world,
            local_matrix.value());
        if (!composed) {
            return Result<Math::Matrix4>::failure(composed.error());
        }
        world = composed.value();
        if (!world.is_finite()) {
            return Result<Math::Matrix4>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "Composing the hierarchy produced a non-finite world matrix"));
        }
    }

    return Result<Math::Matrix4>::success(world);
}

} // namespace Storm
