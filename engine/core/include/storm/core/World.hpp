#pragma once

#include "storm/ecs/Registry.hpp"
#include "storm/math/Transform.hpp"
#include <unordered_map>
#include <vector>

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
        worldCache_.erase(entity.id());
        registry_.destroy(entity);
        ++worldRevision_;
    }

    bool valid(ecs::Entity entity) const noexcept { return registry_.valid(entity); }

    ecs::Registry& registry() noexcept { return registry_; }
    const ecs::Registry& registry() const noexcept { return registry_; }

    math::Transform& addTransform(ecs::Entity entity) {
        worldCache_.erase(entity.id());
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
        worldCache_.erase(entity.id());
        ++worldRevision_;
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

        // Parent changes invalidate only the affected subtree logically: cached
        // children carry the parent's revision and will refresh on demand.
        worldCache_.erase(child.id());
        ++worldRevision_;
        return true;
    }

    void clearParent(ecs::Entity child) {
        if (!child.valid()) return;
        if (parents_.erase(child.id()) != 0) {
            worldCache_.erase(child.id());
            ++worldRevision_;
        }
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
        const std::uint64_t parentRevision = parent.valid() ? worldRevisionOf(parent) : 0;
        const math::Mat4 local = transform->localMatrix();

        auto& cache = worldCache_[entity.id()];
        if (cache.valid && cache.parentRevision == parentRevision &&
            sameMatrix(cache.local, local)) {
            return cache.world;
        }

        const math::Mat4 world = parent.valid()
            ? worldMatrix(parent) * local
            : local;

        cache.local = local;
        cache.world = world;
        cache.parentRevision = parent.valid() ? worldRevisionOf(parent) : 0;
        cache.valid = true;
        cache.revision = ++worldRevision_;
        return world;
    }

private:
    struct WorldCache final {
        math::Mat4 local{};
        math::Mat4 world{};
        std::uint64_t parentRevision{0};
        std::uint64_t revision{0};
        bool valid{false};
    };

    static bool sameMatrix(const math::Mat4& a, const math::Mat4& b) noexcept {
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                if (a.m[row][col] != b.m[row][col]) return false;
        return true;
    }

    std::uint64_t worldRevisionOf(ecs::Entity entity) const {
        // Calling worldMatrix here is intentional: it lazily refreshes an
        // ancestor whose local transform changed without requiring callers to
        // notify World after mutating public Transform fields.
        (void)worldMatrix(entity);
        const auto it = worldCache_.find(entity.id());
        return it == worldCache_.end() ? 0 : it->second.revision;
    }

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
    mutable std::unordered_map<ecs::Entity::Id, WorldCache> worldCache_;
    mutable std::uint64_t worldRevision_{0};
};

} // namespace storm::core
