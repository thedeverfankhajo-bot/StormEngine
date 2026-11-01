#pragma once

#include "storm/core/scene.hpp"
#include "storm/math/transform1d.hpp"
#include "storm/physics/one_d.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace Storm::Physics::OneD {

// Optional Scene bridge. Neither Core nor the standalone Physics1D target
// depends on this adapter. Calls and Scene structural changes are single-thread-affine.
class PhysicsScene1D final {
public:
    [[nodiscard]] static Result<PhysicsScene1D> create(
        Scene& scene,
        PhysicsWorld1DConfig config = {});

    PhysicsScene1D(const PhysicsScene1D&) = delete;
    PhysicsScene1D& operator=(const PhysicsScene1D&) = delete;
    PhysicsScene1D(PhysicsScene1D&& other) noexcept;
    PhysicsScene1D& operator=(PhysicsScene1D&&) = delete;

    // Transform1D position is authoritative; definition.position is ignored.
    [[nodiscard]] Result<BodyHandle1D> attach(
        Scene& scene,
        EntityHandle entity,
        const BodyDefinition1D& definition = {});
    [[nodiscard]] Result<void> detach(Scene& scene, EntityHandle entity);
    [[nodiscard]] Result<void> set_position(Scene& scene, EntityHandle entity, double position);
    [[nodiscard]] Result<void> set_velocity(Scene& scene, EntityHandle entity, double velocity);
    [[nodiscard]] Result<void> apply_force(Scene& scene, EntityHandle entity, double force);
    [[nodiscard]] Result<void> apply_impulse(Scene& scene, EntityHandle entity, double impulse);

    [[nodiscard]] Result<BodySnapshot1D> body_snapshot(
        const Scene& scene, EntityHandle entity) const;
    [[nodiscard]] Result<std::optional<BodyHandle1D>> body_for_entity(
        const Scene& scene, EntityHandle entity) const;
    [[nodiscard]] Result<std::optional<EntityHandle>> entity_for_body(
        const Scene& scene, BodyHandle1D body) const;

    // Retires mappings for destroyed entities or removed Transform1D components,
    // advances physics, then copies successful scalar positions back to Scene.
    [[nodiscard]] Result<StepResult1D> step(
        Scene& scene, double delta_seconds, double gravity = 0.0);

    [[nodiscard]] std::size_t body_count() const noexcept;

private:
    struct Entry final {
        EntityHandle entity{};
        BodyHandle1D body{};
    };

    PhysicsScene1D(SceneId scene_id, PhysicsWorld1D world) noexcept;
    [[nodiscard]] Result<void> validate_scene(const Scene& scene) const;
    [[nodiscard]] Entry* find_entity(EntityHandle entity) noexcept;
    [[nodiscard]] const Entry* find_entity(EntityHandle entity) const noexcept;
    [[nodiscard]] Entry* find_body(BodyHandle1D body) noexcept;
    [[nodiscard]] const Entry* find_body(BodyHandle1D body) const noexcept;

    SceneId scene_id_{0U};
    PhysicsWorld1D world_;
    std::vector<Entry> entries_;
};

} // namespace Storm::Physics::OneD
