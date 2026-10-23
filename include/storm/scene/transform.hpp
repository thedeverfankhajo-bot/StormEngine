#pragma once

#include "storm/math/matrix4.hpp"
#include "storm/math/vector.hpp"
#include "storm/scene/dimension.hpp"

#include <cmath>
#include <optional>
#include <variant>

namespace storm::scene {

// Dimension-aware local transform values for the unified 1D/2D/3D scene model.
// Rotation values are expressed in radians; world and matrix conventions are documented in ADR 0002.
struct Transform1D final {
    float position{};
    float scale{1.0F};

    [[nodiscard]] bool is_finite() const noexcept {
        return std::isfinite(position) && std::isfinite(scale);
    }
};

struct Transform2D final {
    math::Vec2 position{};
    float rotation_radians{};
    math::Vec2 scale{1.0F, 1.0F};

    [[nodiscard]] bool is_finite() const noexcept {
        return position.is_finite() && std::isfinite(rotation_radians) && scale.is_finite();
    }
};

struct Transform3D final {
    math::Vec3 position{};
    math::Vec3 rotation_euler_radians{};
    math::Vec3 scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool is_finite() const noexcept {
        return position.is_finite() && rotation_euler_radians.is_finite() && scale.is_finite();
    }
};

using TransformData = std::variant<Transform1D, Transform2D, Transform3D>;

// Holds exactly one dimensional transform. Invalid enum values never silently become 1D/2D/3D.
class Transform final {
public:
    Transform() noexcept = default;

    explicit Transform(Transform1D value) noexcept : data_(value) {}
    explicit Transform(Transform2D value) noexcept : data_(value) {}
    explicit Transform(Transform3D value) noexcept : data_(value) {}

    [[nodiscard]] static std::optional<Transform> for_dimension(Dimension dimension) noexcept {
        switch (dimension) {
        case Dimension::One:
            return Transform{Transform1D{}};
        case Dimension::Two:
            return Transform{Transform2D{}};
        case Dimension::Three:
            return Transform{Transform3D{}};
        }
        return std::nullopt;
    }

    [[nodiscard]] Dimension dimension() const noexcept {
        if (std::holds_alternative<Transform1D>(data_)) {
            return Dimension::One;
        }
        if (std::holds_alternative<Transform2D>(data_)) {
            return Dimension::Two;
        }
        return Dimension::Three;
    }

    [[nodiscard]] bool is_finite() const noexcept {
        return std::visit([](const auto& value) { return value.is_finite(); }, data_);
    }

    [[nodiscard]] const TransformData& data() const noexcept {
        return data_;
    }

    // Converts this local transform to the engine's column-vector Matrix4 convention.
    // 1D maps onto X; 2D maps onto the XY plane; 3D applies scale, then X/Y/Z rotations,
    // then translation. Returns no matrix for invalid or numerically overflowing values.
    [[nodiscard]] std::optional<math::Matrix4> to_matrix() const noexcept {
        if (!is_finite()) {
            return std::nullopt;
        }

        math::Matrix4 result;
        if (const auto* one = std::get_if<Transform1D>(&data_)) {
            result = math::Matrix4::translation(math::Vec3{one->position, 0.0F, 0.0F}) *
                     math::Matrix4::scale(math::Vec3{one->scale, 1.0F, 1.0F});
        } else if (const auto* two = std::get_if<Transform2D>(&data_)) {
            result = math::Matrix4::translation(
                         math::Vec3{two->position[0U], two->position[1U], 0.0F}) *
                     math::Matrix4::rotation_z(two->rotation_radians) *
                     math::Matrix4::scale(
                         math::Vec3{two->scale[0U], two->scale[1U], 1.0F});
        } else {
            const auto& three = std::get<Transform3D>(data_);
            result = math::Matrix4::translation(three.position) *
                     math::Matrix4::rotation_z(three.rotation_euler_radians[2U]) *
                     math::Matrix4::rotation_y(three.rotation_euler_radians[1U]) *
                     math::Matrix4::rotation_x(three.rotation_euler_radians[0U]) *
                     math::Matrix4::scale(three.scale);
        }

        if (!result.is_finite()) {
            return std::nullopt;
        }
        return result;
    }

private:
    TransformData data_{Transform3D{}};
};

} // namespace storm::scene
