#pragma once

#include "Mat4.hpp"
#include "Quaternion.hpp"

namespace storm::math {

struct Transform {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Quaternion rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Mat4 localMatrix() const {
        return Mat4::translation(position) * rotation.toMat4() * Mat4::scale(scale);
    }

    Vec3 forward() const { return rotation.rotate(Vec3::forward()); }
    Vec3 right() const { return rotation.rotate(Vec3::right()); }
    Vec3 up() const { return rotation.rotate(Vec3::up()); }
};

} // namespace storm::math
