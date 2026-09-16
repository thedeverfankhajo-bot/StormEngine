#pragma once

#include "storm/ecs/Registry.hpp"
#include "storm/math/Transform.hpp"

namespace storm::core {

class World {
public:
    ecs::Entity createEntity() { return registry_.create(); }

    void destroyEntity(ecs::Entity entity) { registry_.destroy(entity); }

    bool valid(ecs::Entity entity) const noexcept { return registry_.valid(entity); }

    ecs::Registry& registry() noexcept { return registry_; }
    const ecs::Registry& registry() const noexcept { return registry_; }

    math::Transform& addTransform(ecs::Entity entity) {
        return registry_.emplace<math::Transform>(entity);
    }

    math::Transform* tryGetTransform(ecs::Entity entity) {
        return registry_.tryGet<math::Transform>(entity);
    }

    const math::Transform* tryGetTransform(ecs::Entity entity) const {
        return registry_.tryGet<math::Transform>(entity);
    }

    math::Transform& getTransform(ecs::Entity entity) {
        return registry_.get<math::Transform>(entity);
    }

    const math::Transform& getTransform(ecs::Entity entity) const {
        return registry_.get<math::Transform>(entity);
    }

    void removeTransform(ecs::Entity entity) {
        registry_.remove<math::Transform>(entity);
    }

    std::size_t transformCount() const {
        return registry_.componentCount<math::Transform>();
    }

private:
    ecs::Registry registry_;
};

} // namespace storm::core
