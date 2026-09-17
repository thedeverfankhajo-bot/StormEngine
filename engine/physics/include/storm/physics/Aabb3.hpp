#pragma once

#include "storm/math/Vec3.hpp"
#include <algorithm>
#include <cmath>

namespace storm::physics {

struct Aabb3 final {
    math::Vec3 min{};
    math::Vec3 max{};
    constexpr Aabb3() = default;
    constexpr Aabb3(const math::Vec3& minimum, const math::Vec3& maximum) : min(minimum), max(maximum) {}
    [[nodiscard]] constexpr math::Vec3 size() const noexcept { return max - min; }
    [[nodiscard]] constexpr math::Vec3 center() const noexcept { return (min + max) * 0.5f; }
    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z) &&
               std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z) &&
               min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    [[nodiscard]] constexpr bool overlaps(const Aabb3& other) const noexcept {
        return min.x < other.max.x && max.x > other.min.x &&
               min.y < other.max.y && max.y > other.min.y &&
               min.z < other.max.z && max.z > other.min.z;
    }
};

struct Collision3D final {
    bool hit{false};
    math::Vec3 normal{};
    math::Vec3 penetration{};
};

[[nodiscard]] inline Collision3D collide(const Aabb3& a, const Aabb3& b) noexcept {
    if (!a.valid() || !b.valid() || !a.overlaps(b)) return {};
    const float left = b.max.x - a.min.x;
    const float right = a.max.x - b.min.x;
    const float down = b.max.y - a.min.y;
    const float up = a.max.y - b.min.y;
    const float back = b.max.z - a.min.z;
    const float front = a.max.z - b.min.z;
    const float px = std::min(left, right);
    const float py = std::min(down, up);
    const float pz = std::min(back, front);
    const math::Vec3 ca = a.center();
    const math::Vec3 cb = b.center();
    if (px <= py && px <= pz) return {true, {ca.x < cb.x ? -1.0f : 1.0f, 0.0f, 0.0f}, {px, 0.0f, 0.0f}};
    if (py <= pz) return {true, {0.0f, ca.y < cb.y ? -1.0f : 1.0f, 0.0f}, {0.0f, py, 0.0f}};
    return {true, {0.0f, 0.0f, ca.z < cb.z ? -1.0f : 1.0f}, {0.0f, 0.0f, pz}};
}

} // namespace storm::physics
