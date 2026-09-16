#pragma once

#include "storm/ecs/Registry.hpp"
#include "storm/math/Transform.hpp"
#include <unordered_map>

namespace storm::core {

class World {
public:
    ecs::Entity createEntity() { return registry_.create(); }

    void destroyEntity(ecs::Entity entity) {
        if (!registry_.valid(entity)) return;
        clearParent(entity);
        for (auto it = parents_.begin(); it != parents_.end();) {
            if (it->second == entity) it = parents_.erase(it);
            else ++it;
        }
        registry_.destroy(entity);
    }

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

    bool setParent(ecs::Entity child, ecs::Entity parent) {
        if (!registry_.valid(child)) return false;
        if (child == parent) return false;
        if (parent.valid() && !registry_.valid(parent)) return false;
        if (parent.valid() && wouldCreateCycle(child, parent)) return false;

        if (parent.valid()) parents_[child.id()] = parent;
        else parents_.erase(child.id());
        return true;
    }

    void clearParent(ecs::Entity child) {
        if (!child.valid()) return;
        parents_.erase(child.id());
    }

    ecs::Entity parentOf(ecs::Entity child) const noexcept {
        if (!registry_.valid(child)) return {};
        const auto it = parents_.find(child.id());
        if (it == parents_.end() || !registry_.valid(it->second)) return {};
        return it->second;
    }

    bool hasParent(ecs::Entity child) const noexcept {
        return parentOf(child).valid();
    }

    math::Mat4 worldMatrix(ecs::Entity entity) const {
        if (!registry_.valid(entity)) return math::Mat4::identity();
        const auto* transform = registry_.tryGet<math::Transform>(entity);
        if (!transform) return math::Mat4::identity();

        const ecs::Entity parent = parentOf(entity);
        if (!parent.valid()) return transform->localMatrix();
        return worldMatrix(parent) * transform->localMatrix();
    }

private:
    bool wouldCreateCycle(ecs::Entity child, ecs::Entity parent) const noexcept {
        ecs::Entity current = parent;
        while (current.valid()) {
            if (current == child) return true;
            current = parentOf(current);
        }
        return false;
    }

    ecs::Registry registry_;
    std::unordered_map<ecs::Entity::Id, ecs::Entity> parents_;
};

} // namespace storm::core
