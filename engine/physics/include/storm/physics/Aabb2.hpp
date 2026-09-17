#pragma once

#include "storm/math/Vec2.hpp"
#include <algorithm>
#include <cmath>

namespace storm::physics {

struct Aabb2 final {
    math::Vec2 min{};
    math::Vec2 max{};

    constexpr Aabb2() = default;
    constexpr Aabb2(const math::Vec2& minimum, const math::Vec2& maximum)
        : min(minimum), max(maximum) {}

    [[nodiscard]] constexpr math::Vec2 size() const noexcept { return max - min; }
    [[nodiscard]] constexpr math::Vec2 center() const noexcept { return (min + max) * 0.5f; }

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(min.x) && std::isfinite(min.y) &&
               std::isfinite(max.x) && std::isfinite(max.y) &&
               min.x <= max.x && min.y <= max.y;
    }

    [[nodiscard]] constexpr bool overlaps(const Aabb2& other) const noexcept {
        return min.x < other.max.x && max.x > other.min.x &&
               min.y < other.max.y && max.y > other.min.y;
    }
};

struct Collision2D final {
    bool hit{false};
    math::Vec2 normal{};
    math::Vec2 penetration{};
};

[[nodiscard]] inline Collision2D collide(const Aabb2& a, const Aabb2& b) noexcept {
    if (!a.valid() || !b.valid() || !a.overlaps(b)) return {};

    const float left = b.max.x - a.min.x;
    const float right = a.max.x - b.min.x;
    const float down = b.max.y - a.min.y;
    const float up = a.max.y - b.min.y;

    const float xPenetration = std::min(left, right);
    const float yPenetration = std::min(down, up);
    if (xPenetration < yPenetration) {
        return {true, {a.center().x < b.center().x ? -1.0f : 1.0f, 0.0f}, {xPenetration, 0.0f}};
    }
    return {true, {0.0f, a.center().y < b.center().y ? -1.0f : 1.0f}, {0.0f, yPenetration}};
}

} // namespace storm::physics
