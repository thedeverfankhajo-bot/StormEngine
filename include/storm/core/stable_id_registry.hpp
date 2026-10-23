#pragma once

#include "storm/core/stable_id.hpp"
#include "storm/core/status.hpp"

#include <cstddef>
#include <set>

namespace storm::core {

// Deterministic bytewise ordering for stable IDs; no attacker-controlled hash collisions.
struct StableIdLess final {
    [[nodiscard]] bool operator()(const StableId& left, const StableId& right) const noexcept {
        const auto& left_bytes = left.bytes();
        const auto& right_bytes = right.bytes();

        for (std::size_t index = 0U; index < StableId::byte_count; ++index) {
            if (left_bytes[index] != right_bytes[index]) {
                return left_bytes[index] < right_bytes[index];
            }
        }
        return false;
    }
};

// In-memory duplicate detector for IDs discovered while loading a project or asset index.
// It does not generate IDs, own the resources identified by them, or provide synchronization.
class StableIdRegistry final {
public:
    [[nodiscard]] Status register_id(const StableId& id) {
        if (!id.is_valid()) {
            return Status::failure(
            ErrorCode::InvalidArgument, "cannot register an invalid stable ID");
        }

        const bool inserted = ids_.insert(id).second;
        if (!inserted) {
            return Status::failure(ErrorCode::AlreadyExists, "stable ID is already registered");
        }
        return Status::success();
    }

    [[nodiscard]] Status unregister_id(const StableId& id) {
        if (!id.is_valid()) {
            return Status::failure(
                ErrorCode::InvalidArgument, "cannot unregister an invalid stable ID");
        }

        const auto found = ids_.find(id);
        if (found == ids_.end()) {
            return Status::failure(ErrorCode::NotFound, "stable ID is not registered");
        }

        ids_.erase(found);
        return Status::success();
    }

    [[nodiscard]] bool contains(const StableId& id) const {
        return id.is_valid() && ids_.find(id) != ids_.end();
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return ids_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return ids_.empty();
    }

    void clear() noexcept {
        ids_.clear();
    }

private:
    std::set<StableId, StableIdLess> ids_{};
};

} // namespace storm::core
