#include "storm/math/transform3d.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Storm::Math {
namespace {

constexpr double kMinimumMagnitudeSquared = 1.0e-24;

[[nodiscard]] Error invalid_math_input(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

} // namespace

bool Vector3::is_finite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

Quaternion Quaternion::identity() noexcept {
    return Quaternion{};
}

bool Quaternion::is_finite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) &&
        std::isfinite(z) && std::isfinite(w);
}

Result<Quaternion> Quaternion::normalized() const {
    if (!is_finite()) {
        return Result<Quaternion>::failure(
            invalid_math_input("A quaternion must contain only finite values"));
    }

    const double dx = static_cast<double>(x);
    const double dy = static_cast<double>(y);
    const double dz = static_cast<double>(z);
    const double dw = static_cast<double>(w);
    const double length_squared =
        (dx * dx) + (dy * dy) + (dz * dz) + (dw * dw);
    if (!std::isfinite(length_squared) ||
        length_squared <= kMinimumMagnitudeSquared) {
        return Result<Quaternion>::failure(
            invalid_math_input("A quaternion must have non-zero finite magnitude"));
    }

    const double inverse_length = 1.0 / std::sqrt(length_squared);
    return Result<Quaternion>::success(Quaternion{
        .x = static_cast<float>(dx * inverse_length),
        .y = static_cast<float>(dy * inverse_length),
        .z = static_cast<float>(dz * inverse_length),
        .w = static_cast<float>(dw * inverse_length),
    });
}

Result<Quaternion> Quaternion::from_axis_angle(
    Vector3 axis,
    float angle_radians) {
    if (!axis.is_finite() || !std::isfinite(angle_radians)) {
        return Result<Quaternion>::failure(
            invalid_math_input("Axis-angle input must contain only finite values"));
    }

    const double dx = static_cast<double>(axis.x);
    const double dy = static_cast<double>(axis.y);
    const double dz = static_cast<double>(axis.z);
    const double length_squared = (dx * dx) + (dy * dy) + (dz * dz);
    if (!std::isfinite(length_squared) ||
        length_squared <= kMinimumMagnitudeSquared) {
        return Result<Quaternion>::failure(
            invalid_math_input("An axis-angle rotation requires a non-zero axis"));
    }

    const double inverse_axis_length = 1.0 / std::sqrt(length_squared);
    const double half_angle = static_cast<double>(angle_radians) * 0.5;
    const double sine = std::sin(half_angle);
    const double cosine = std::cos(half_angle);

    const Quaternion candidate{
        .x = static_cast<float>(dx * inverse_axis_length * sine),
        .y = static_cast<float>(dy * inverse_axis_length * sine),
        .z = static_cast<float>(dz * inverse_axis_length * sine),
        .w = static_cast<float>(cosine),
    };
    return candidate.normalized();
}

Matrix4 Matrix4::identity() noexcept {
    Matrix4 matrix;
    matrix.values_[0U] = 1.0F;
    matrix.values_[5U] = 1.0F;
    matrix.values_[10U] = 1.0F;
    matrix.values_[15U] = 1.0F;
    return matrix;
}

float Matrix4::at(std::size_t row, std::size_t column) const {
    if (row >= 4U || column >= 4U) {
        throw std::out_of_range("Matrix4 indices must be in the range [0, 4)");
    }
    return values_[(column * 4U) + row];
}

bool Matrix4::is_finite() const noexcept {
    return std::all_of(
        values_.begin(),
        values_.end(),
        [](float value) { return std::isfinite(value); });
}

Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs) noexcept {
    Matrix4 result;
    for (std::size_t column = 0U; column < 4U; ++column) {
        for (std::size_t row = 0U; row < 4U; ++row) {
            double value = 0.0;
            for (std::size_t inner = 0U; inner < 4U; ++inner) {
                value +=
                    static_cast<double>(lhs.values_[(inner * 4U) + row]) *
                    static_cast<double>(rhs.values_[(column * 4U) + inner]);
            }
            result.values_[(column * 4U) + row] = static_cast<float>(value);
        }
    }
    return result;
}

bool Transform3D::is_valid() const noexcept {
    if (!translation.is_finite() || !rotation.is_finite() || !scale.is_finite()) {
        return false;
    }

    const double qx = static_cast<double>(rotation.x);
    const double qy = static_cast<double>(rotation.y);
    const double qz = static_cast<double>(rotation.z);
    const double qw = static_cast<double>(rotation.w);
    const double length_squared =
        (qx * qx) + (qy * qy) + (qz * qz) + (qw * qw);
    return std::isfinite(length_squared) &&
        length_squared > kMinimumMagnitudeSquared;
}

Result<Matrix4> Transform3D::to_matrix() const {
    if (!translation.is_finite() || !scale.is_finite()) {
        return Result<Matrix4>::failure(
            invalid_math_input("Transform translation and scale must be finite"));
    }

    const Result<Quaternion> normalized_rotation = rotation.normalized();
    if (!normalized_rotation) {
        return Result<Matrix4>::failure(normalized_rotation.error());
    }

    const Quaternion q = normalized_rotation.value();
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;

    const float r00 = 1.0F - (2.0F * (yy + zz));
    const float r01 = 2.0F * (xy - wz);
    const float r02 = 2.0F * (xz + wy);
    const float r10 = 2.0F * (xy + wz);
    const float r11 = 1.0F - (2.0F * (xx + zz));
    const float r12 = 2.0F * (yz - wx);
    const float r20 = 2.0F * (xz - wy);
    const float r21 = 2.0F * (yz + wx);
    const float r22 = 1.0F - (2.0F * (xx + yy));

    Matrix4 matrix = Matrix4::identity();
    matrix.values_[0U] = r00 * scale.x;
    matrix.values_[1U] = r10 * scale.x;
    matrix.values_[2U] = r20 * scale.x;
    matrix.values_[4U] = r01 * scale.y;
    matrix.values_[5U] = r11 * scale.y;
    matrix.values_[6U] = r21 * scale.y;
    matrix.values_[8U] = r02 * scale.z;
    matrix.values_[9U] = r12 * scale.z;
    matrix.values_[10U] = r22 * scale.z;
    matrix.values_[12U] = translation.x;
    matrix.values_[13U] = translation.y;
    matrix.values_[14U] = translation.z;

    if (!matrix.is_finite()) {
        return Result<Matrix4>::failure(
            invalid_math_input("The transform exceeds the representable matrix range"));
    }
    return Result<Matrix4>::success(matrix);
}

} // namespace Storm::Math
