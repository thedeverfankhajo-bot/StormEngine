#pragma once

#include <algorithm>
#include <cmath>

namespace storm::physics {

struct Aabb1 final {
    float min{0.0f};
    float max{0.0f};
    constexpr Aabb1() = default;
    constexpr Aabb1(float minimum, float maximum) : min(minimum), max(maximum) {}
    [[nodiscard]] constexpr float size() const noexcept { return max - min; }
    [[nodiscard]] constexpr float center() const noexcept { return (min + max) * 0.5f; }
    [[nodiscard]] bool valid() const noexcept { return std::isfinite(min) && std::isfinite(max) && min <= max; }
    [[nodiscard]] constexpr bool overlaps(const Aabb1& other) const noexcept { return min < other.max && max > other.min; }
};

struct Collision1D final {
    bool hit{false};
    float normal{0.0f};
    float penetration{0.0f};
};

[[nodiscard]] inline Collision1D collide(const Aabb1& a, const Aabb1& b) noexcept {
    if (!a.valid() || !b.valid() || !a.overlaps(b)) return {};
    const float left = b.max - a.min;
    const float right = a.max - b.min;
    const float penetration = std::min(left, right);
    return {true, a.center() < b.center() ? -1.0f : 1.0f, penetration};
}

} // namespace storm::physics
