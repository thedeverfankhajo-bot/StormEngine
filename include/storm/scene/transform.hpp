#pragma once

#include "storm/math/vector.hpp"
#include "storm/scene/dimension.hpp"

#include <cmath>
#include <optional>
#include <variant>

namespace storm::scene {

// Provisional transform values for a unified 1D/2D/3D scene model.
// Angles are expressed in radians; renderer-facing axis/handedness conventions remain a
// documented architecture decision before rendering and physics depend on them.
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

private:
    TransformData data_{Transform3D{}};
};

} // namespace storm::scene
