#pragma once

#include "Mat4.hpp"
#include "Quaternion.hpp"
#include "Vec3.hpp"
#include <cmath>

namespace storm::math {

struct Transform3D final {
    Vec3 position{};
    Quaternion rotation{Quaternion::identity()};
    Vec3 scale{Vec3::one()};

    [[nodiscard]] Mat4 matrix() const {
        return Mat4::translation(position) * rotation.toMat4() * Mat4::scale(scale);
    }

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
               std::isfinite(rotation.x) && std::isfinite(rotation.y) && std::isfinite(rotation.z) && std::isfinite(rotation.w) &&
               std::isfinite(scale.x) && std::isfinite(scale.y) && std::isfinite(scale.z);
    }
};

} // namespace storm::math
