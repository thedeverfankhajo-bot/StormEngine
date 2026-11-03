#pragma once

#include "storm/core/scene.hpp"
#include "storm/math/transform2d.hpp"
#include "storm/physics/two_d.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace Storm::Physics::TwoD {

// Optional bridge between the independent Physics2D world and Scene.
// Physics uses the local Transform2D translation only: parented entities are
// rejected, and rotation/scale are preserved but do not affect body geometry.
// Calls and Scene structural changes are single-thread-affine. Call detach()
// before removing and re-adding Transform2D between adapter steps.
class PhysicsScene2D final {
public:
    [[nodiscard]] static Result<PhysicsScene2D> create(
        Scene& scene,
        PhysicsWorld2DConfig config = {});

    PhysicsScene2D(const PhysicsScene2D&) = delete;
    PhysicsScene2D& operator=(const PhysicsScene2D&) = delete;
    PhysicsScene2D(PhysicsScene2D&& other) noexcept;
    PhysicsScene2D& operator=(PhysicsScene2D&&) = delete;

    // Transform2D::translation is authoritative; definition.position is ignored.
    [[nodiscard]] Result<BodyHandle2D> attach(
        Scene& scene,
        EntityHandle entity,
        const BodyDefinition2D& definition = {});
    [[nodiscard]] Result<void> detach(Scene& scene, EntityHandle entity);
    [[nodiscard]] Result<void> set_position(
        Scene& scene, EntityHandle entity, Vector2 position);
    [[nodiscard]] Result<void> set_velocity(
        Scene& scene, EntityHandle entity, Vector2 velocity);
    [[nodiscard]] Result<void> apply_force(
        Scene& scene, EntityHandle entity, Vector2 force);
    [[nodiscard]] Result<void> apply_impulse(
        Scene& scene, EntityHandle entity, Vector2 impulse);

    [[nodiscard]] Result<BodySnapshot2D> body_snapshot(
        const Scene& scene, EntityHandle entity) const;
    [[nodiscard]] Result<std::optional<BodyHandle2D>> body_for_entity(
        const Scene& scene, EntityHandle entity) const;
    [[nodiscard]] Result<std::optional<EntityHandle>> entity_for_body(
        const Scene& scene, BodyHandle2D body) const;

    // Retires destroyed entities and removed Transform2D components, advances
    // physics, then synchronizes successful positions back to Scene.
    [[nodiscard]] Result<StepResult2D> step(
        Scene& scene,
        double delta_seconds,
        Vector2 gravity = {});

    [[nodiscard]] std::size_t body_count() const noexcept;

private:
    struct Entry final {
        EntityHandle entity{};
        BodyHandle2D body{};
    };

    PhysicsScene2D(SceneId scene_id, PhysicsWorld2D world) noexcept;
    [[nodiscard]] Result<void> validate_scene(const Scene& scene) const;
    [[nodiscard]] Result<void> validate_entity_transform(
        const Scene& scene, EntityHandle entity) const;
    [[nodiscard]] Entry* find_entity(EntityHandle entity) noexcept;
    [[nodiscard]] const Entry* find_entity(EntityHandle entity) const noexcept;
    [[nodiscard]] Entry* find_body(BodyHandle2D body) noexcept;
    [[nodiscard]] const Entry* find_body(BodyHandle2D body) const noexcept;

    SceneId scene_id_{0U};
    PhysicsWorld2D world_;
    std::vector<Entry> entries_;
};

} // namespace Storm::Physics::TwoD
