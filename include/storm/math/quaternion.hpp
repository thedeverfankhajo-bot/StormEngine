#pragma once

#include "storm/math/matrix4.hpp"
#include "storm/math/vector.hpp"

#include <cmath>
#include <optional>

namespace storm::math {

// A right-handed 3D rotation value. Components are stored as (w, x, y, z).
// Quaternions use active rotations and compose right-to-left: (a * b) rotates by b first,
// then by a. Matrix conversion follows Matrix4's column-vector convention.
class Quaternion final {
public:
    static constexpr float default_epsilon = 1.0e-6F;

    // The identity rotation, which leaves every vector unchanged.
    constexpr Quaternion() noexcept = default;

    // Components are intentionally not normalized automatically. Use normalized(), or a
    // validated factory, when a unit quaternion is required by a rotation operation.
    constexpr Quaternion(float w, float x, float y, float z) noexcept
        : w_(w), x_(x), y_(y), z_(z) {}

    [[nodiscard]] static std::optional<Quaternion> from_axis_angle(
        const Vec3& axis, float radians, float epsilon = default_epsilon) noexcept {
        if (!std::isfinite(radians) || !std::isfinite(epsilon) || epsilon < 0.0F) {
            return std::nullopt;
        }

        const auto unit_axis = axis.normalized(epsilon);
        if (!unit_axis.has_value()) {
            return std::nullopt;
        }

        const float half_angle = radians * 0.5F;
        const float sine = std::sin(half_angle);
        const float cosine = std::cos(half_angle);
        if (!std::isfinite(sine) || !std::isfinite(cosine)) {
            return std::nullopt;
        }

        const Quaternion result{cosine, (*unit_axis)[0U] * sine,
                                (*unit_axis)[1U] * sine, (*unit_axis)[2U] * sine};
        return result.is_finite() ? std::optional<Quaternion>{result} : std::nullopt;
    }

    [[nodiscard]] constexpr float w() const noexcept { return w_; }
    [[nodiscard]] constexpr float x() const noexcept { return x_; }
    [[nodiscard]] constexpr float y() const noexcept { return y_; }
    [[nodiscard]] constexpr float z() const noexcept { return z_; }

    [[nodiscard]] constexpr float length_squared() const noexcept {
        return w_ * w_ + x_ * x_ + y_ * y_ + z_ * z_;
    }

    [[nodiscard]] bool is_finite() const noexcept {
        return std::isfinite(w_) && std::isfinite(x_) && std::isfinite(y_) && std::isfinite(z_);
    }

    // Rejects non-finite inputs, negative epsilon and values too close to zero.
    [[nodiscard]] std::optional<Quaternion> normalized(
        float epsilon = default_epsilon) const noexcept {
        if (!is_finite() || !std::isfinite(epsilon) || epsilon < 0.0F) {
            return std::nullopt;
        }

        const float squared_length = length_squared();
        if (!std::isfinite(squared_length) || squared_length <= epsilon * epsilon) {
            return std::nullopt;
        }

        const float magnitude = std::sqrt(squared_length);
        if (!std::isfinite(magnitude) || magnitude <= epsilon) {
            return std::nullopt;
        }

        const Quaternion result{w_ / magnitude, x_ / magnitude, y_ / magnitude, z_ / magnitude};
        return result.is_finite() ? std::optional<Quaternion>{result} : std::nullopt;
    }

    [[nodiscard]] constexpr Quaternion conjugated() const noexcept {
        return Quaternion{w_, -x_, -y_, -z_};
    }

    [[nodiscard]] constexpr Quaternion operator*(const Quaternion& other) const noexcept {
        return Quaternion{
            w_ * other.w_ - x_ * other.x_ - y_ * other.y_ - z_ * other.z_,
            w_ * other.x_ + x_ * other.w_ + y_ * other.z_ - z_ * other.y_,
            w_ * other.y_ - x_ * other.z_ + y_ * other.w_ + z_ * other.x_,
            w_ * other.z_ + x_ * other.y_ - y_ * other.x_ + z_ * other.w_};
    }

    // Applies this rotation to a direction. Non-unit quaternions are normalized first;
    // invalid quaternions, directions or results return no value.
    [[nodiscard]] std::optional<Vec3> rotate_vector(const Vec3& direction) const noexcept {
        if (!direction.is_finite()) {
            return std::nullopt;
        }
        const auto unit = normalized();
        if (!unit.has_value()) {
            return std::nullopt;
        }

        const Vec3 imaginary{unit->x_, unit->y_, unit->z_};
        const Vec3 first_cross{
            imaginary[1U] * direction[2U] - imaginary[2U] * direction[1U],
            imaginary[2U] * direction[0U] - imaginary[0U] * direction[2U],
            imaginary[0U] * direction[1U] - imaginary[1U] * direction[0U]};
        const Vec3 inner{first_cross[0U] + unit->w_ * direction[0U],
                         first_cross[1U] + unit->w_ * direction[1U],
                         first_cross[2U] + unit->w_ * direction[2U]};
        const Vec3 second_cross{
            imaginary[1U] * inner[2U] - imaginary[2U] * inner[1U],
            imaginary[2U] * inner[0U] - imaginary[0U] * inner[2U],
            imaginary[0U] * inner[1U] - imaginary[1U] * inner[0U]};

        const Vec3 result{direction[0U] + 2.0F * second_cross[0U],
                          direction[1U] + 2.0F * second_cross[1U],
                          direction[2U] + 2.0F * second_cross[2U]};
        return result.is_finite() ? std::optional<Vec3>{result} : std::nullopt;
    }

    // Converts a valid rotation to a column-vector Matrix4. The returned matrix has no
    // translation and leaves homogeneous W unchanged. Non-unit values are normalized first.
    [[nodiscard]] std::optional<Matrix4> to_matrix() const noexcept {
        const auto unit = normalized();
        if (!unit.has_value()) {
            return std::nullopt;
        }

        const float xx = unit->x_ * unit->x_;
        const float yy = unit->y_ * unit->y_;
        const float zz = unit->z_ * unit->z_;
        const float xy = unit->x_ * unit->y_;
        const float xz = unit->x_ * unit->z_;
        const float yz = unit->y_ * unit->z_;
        const float wx = unit->w_ * unit->x_;
        const float wy = unit->w_ * unit->y_;
        const float wz = unit->w_ * unit->z_;

        Matrix4 result;
        result(0U, 0U) = 1.0F - 2.0F * (yy + zz);
        result(0U, 1U) = 2.0F * (xy - wz);
        result(0U, 2U) = 2.0F * (xz + wy);
        result(1U, 0U) = 2.0F * (xy + wz);
        result(1U, 1U) = 1.0F - 2.0F * (xx + zz);
        result(1U, 2U) = 2.0F * (yz - wx);
        result(2U, 0U) = 2.0F * (xz - wy);
        result(2U, 1U) = 2.0F * (yz + wx);
        result(2U, 2U) = 1.0F - 2.0F * (xx + yy);
        result(3U, 3U) = 1.0F;

        return result.is_finite() ? std::optional<Matrix4>{result} : std::nullopt;
    }

    [[nodiscard]] constexpr bool operator==(const Quaternion&) const noexcept = default;

private:
    float w_{1.0F};
    float x_{};
    float y_{};
    float z_{};
};

} // namespace storm::math
