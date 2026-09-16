#pragma once

#include <cstdint>

namespace storm::ecs {

class Entity {
public:
    using Id = std::uint32_t;
    using Generation = std::uint32_t;

    constexpr Entity() noexcept = default;
    constexpr Entity(Id id, Generation generation) noexcept : id_(id), generation_(generation) {}

    constexpr Id id() const noexcept { return id_; }
    constexpr Generation generation() const noexcept { return generation_; }
    constexpr bool valid() const noexcept { return id_ != kInvalidId; }

    friend constexpr bool operator==(Entity a, Entity b) noexcept {
        return a.id_ == b.id_ && a.generation_ == b.generation_;
    }
    friend constexpr bool operator!=(Entity a, Entity b) noexcept { return !(a == b); }

    static constexpr Id kInvalidId = 0xFFFFFFFFu;

private:
    Id id_{kInvalidId};
    Generation generation_{0};
};

} // namespace storm::ecs
