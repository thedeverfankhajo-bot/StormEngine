#pragma once

#include "Entity.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <vector>

namespace storm::ecs {

class SparseSet {
public:
    static constexpr std::size_t kInvalidPosition = std::numeric_limits<std::size_t>::max();

    bool contains(Entity::Id entity) const noexcept {
        return entity < sparse_.size() && sparse_[entity] != kInvalidPosition;
    }

    std::size_t size() const noexcept { return dense_.size(); }

    Entity::Id entityAt(std::size_t position) const noexcept {
        assert(position < dense_.size());
        return dense_[position];
    }

    std::size_t position(Entity::Id entity) const noexcept {
        return entity < sparse_.size() ? sparse_[entity] : kInvalidPosition;
    }

    void insert(Entity::Id entity) {
        if (contains(entity)) return;
        ensureSparse(entity);

        // Append first. If vector growth throws, sparse_[entity] remains invalid
        // and the set is unchanged. Only the noexcept index assignment follows.
        const std::size_t position = dense_.size();
        dense_.push_back(entity);
        sparse_[entity] = position;
    }

    void erase(Entity::Id entity) {
        if (!contains(entity)) return;

        const std::size_t removed = sparse_[entity];
        const std::size_t last = dense_.size() - 1;
        const Entity::Id lastEntity = dense_[last];

        dense_[removed] = lastEntity;
        sparse_[lastEntity] = removed;
        dense_.pop_back();
        sparse_[entity] = kInvalidPosition;
    }

    void clear() noexcept {
        dense_.clear();
        std::fill(sparse_.begin(), sparse_.end(), kInvalidPosition);
    }

private:
    void ensureSparse(Entity::Id entity) {
        if (entity >= sparse_.size())
            sparse_.resize(static_cast<std::size_t>(entity) + 1, kInvalidPosition);
    }

    std::vector<std::size_t> sparse_;
    std::vector<Entity::Id> dense_;
};

} // namespace storm::ecs
