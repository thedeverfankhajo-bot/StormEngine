#pragma once

#include "SparseSet.hpp"
#include <utility>
#include <vector>

namespace storm::ecs {

template <typename T>
class ComponentStorage {
public:
    template <typename... Args>
    T& emplace(Entity::Id entity, Args&&... args) {
        const std::size_t existing = entities_.position(entity);
        if (existing != SparseSet::kInvalidPosition) {
            T value(std::forward<Args>(args)...);
            components_[existing] = std::move(value);
            return components_[existing];
        }

        // Construct the component before mutating the sparse set. If construction
        // or vector growth throws, the storage remains unchanged.
        T value(std::forward<Args>(args)...);
        components_.push_back(std::move(value));
        try {
            entities_.insert(entity);
        } catch (...) {
            components_.pop_back();
            throw;
        }
        return components_.back();
    }

    bool has(Entity::Id entity) const noexcept { return entities_.contains(entity); }

    T* tryGet(Entity::Id entity) noexcept {
        const std::size_t position = entities_.position(entity);
        return position == SparseSet::kInvalidPosition ? nullptr : &components_[position];
    }

    const T* tryGet(Entity::Id entity) const noexcept {
        const std::size_t position = entities_.position(entity);
        return position == SparseSet::kInvalidPosition ? nullptr : &components_[position];
    }

    void remove(Entity::Id entity) {
        const std::size_t position = entities_.position(entity);
        if (position == SparseSet::kInvalidPosition) return;

        const std::size_t last = components_.size() - 1;
        if (position != last)
            components_[position] = std::move(components_[last]);
        components_.pop_back();
        entities_.erase(entity);
    }

    std::size_t size() const noexcept { return entities_.size(); }
    Entity::Id entityAt(std::size_t position) const noexcept { return entities_.entityAt(position); }

    void clear() noexcept {
        entities_.clear();
        components_.clear();
    }

private:
    SparseSet entities_;
    std::vector<T> components_;
};

} // namespace storm::ecs
