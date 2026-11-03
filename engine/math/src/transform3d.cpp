#include "storm/math/transform3d.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Storm::Math {
namespace {

constexpr double kMinimumMagnitudeSquared = 1.0e-24;
constexpr double kMinimumHomogeneousMagnitude = 1.0e-12;
constexpr double kPi = 3.14159265358979323846264338327950288;

struct DoubleVector3 final {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

[[nodiscard]] Error invalid_math_input(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

[[nodiscard]] bool is_representable_float(double value) noexcept {
    constexpr double maximum =
        static_cast<double>(std::numeric_limits<float>::max());
    return std::isfinite(value) && value <= maximum && value >= -maximum;
}

[[nodiscard]] Result<float> narrow_finite_float(
    double value,
    const char* error_message) {
    if (!is_representable_float(value)) {
        return Result<float>::failure(invalid_math_input(error_message));
    }
    return Result<float>::success(static_cast<float>(value));
}

[[nodiscard]] DoubleVector3 to_double(Vector3 value) noexcept {
    return DoubleVector3{
        static_cast<double>(value.x),
        static_cast<double>(value.y),
        static_cast<double>(value.z),
    };
}

[[nodiscard]] double dot_double(DoubleVector3 lhs, DoubleVector3 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

[[nodiscard]] DoubleVector3 subtract_double(
    DoubleVector3 lhs,
    DoubleVector3 rhs) noexcept {
    return DoubleVector3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] DoubleVector3 cross_double(
    DoubleVector3 lhs,
    DoubleVector3 rhs) noexcept {
    return DoubleVector3{
        (lhs.y * rhs.z) - (lhs.z * rhs.y),
        (lhs.z * rhs.x) - (lhs.x * rhs.z),
        (lhs.x * rhs.y) - (lhs.y * rhs.x),
    };
}

[[nodiscard]] Result<DoubleVector3> normalize_double(
    DoubleVector3 value,
    const char* error_message) {
    const double magnitude_squared = dot_double(value, value);
    if (!std::isfinite(magnitude_squared) ||
        magnitude_squared <= kMinimumMagnitudeSquared) {
        return Result<DoubleVector3>::failure(invalid_math_input(error_message));
    }

    const double inverse_magnitude = 1.0 / std::sqrt(magnitude_squared);
    const DoubleVector3 result{
        value.x * inverse_magnitude,
        value.y * inverse_magnitude,
        value.z * inverse_magnitude,
    };
    if (!std::isfinite(result.x) ||
        !std::isfinite(result.y) ||
        !std::isfinite(result.z)) {
        return Result<DoubleVector3>::failure(invalid_math_input(error_message));
    }
    return Result<DoubleVector3>::success(result);
}

[[nodiscard]] Result<Vector3> narrow_vector(
    DoubleVector3 value,
    const char* error_message) {
    const auto x = narrow_finite_float(value.x, error_message);
    const auto y = narrow_finite_float(value.y, error_message);
    const auto z = narrow_finite_float(value.z, error_message);
    if (!x) {
        return Result<Vector3>::failure(x.error());
    }
    if (!y) {
        return Result<Vector3>::failure(y.error());
    }
    if (!z) {
        return Result<Vector3>::failure(z.error());
    }
    return Result<Vector3>::success(Vector3{x.value(), y.value(), z.value()});
}

} // namespace

bool Vector3::is_finite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

double Vector3::length_squared() const noexcept {
    const DoubleVector3 value = to_double(*this);
    return dot(value, value);
}

double Vector3::length() const noexcept {
    return std::sqrt(length_squared());
}

double Vector3::dot(Vector3 other) const noexcept {
    return dot_double(to_double(*this), to_double(other));
}

Result<Vector3> Vector3::cross(Vector3 other) const {
    if (!is_finite() || !other.is_finite()) {
        return Result<Vector3>::failure(
            invalid_math_input("Vector3 cross-product inputs must be finite"));
    }
    return narrow_vector(
        cross_double(to_double(*this), to_double(other)),
        "Vector3 cross product exceeds the representable numeric range");
}

Result<Vector3> Vector3::normalized() const {
    if (!is_finite()) {
        return Result<Vector3>::failure(
            invalid_math_input("A Vector3 must contain only finite values"));
    }
    const auto unit = normalize_double(
        to_double(*this),
        "A Vector3 must have non-zero finite magnitude");
    if (!unit) {
        return Result<Vector3>::failure(unit.error());
    }
    return narrow_vector(
        unit.value(),
        "Normalized Vector3 exceeds the representable numeric range");
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

Result<Matrix4> Matrix4::perspective_rh_zo(
    float vertical_fov_radians,
    float aspect_ratio,
    float near_plane,
    float far_plane) {
    if (!std::isfinite(vertical_fov_radians) ||
        !std::isfinite(aspect_ratio) ||
        !std::isfinite(near_plane) ||
        !std::isfinite(far_plane) ||
        vertical_fov_radians <= 0.0F ||
        static_cast<double>(vertical_fov_radians) >= kPi ||
        aspect_ratio <= 0.0F ||
        near_plane <= 0.0F ||
        far_plane <= near_plane) {
        return Result<Matrix4>::failure(invalid_math_input(
            "Perspective requires 0 < vertical FOV < pi, positive aspect and near, and far > near"));
    }

    const double half_fov =
        static_cast<double>(vertical_fov_radians) * 0.5;
    const double tangent = std::tan(half_fov);
    if (!std::isfinite(tangent) || tangent <= 0.0) {
        return Result<Matrix4>::failure(invalid_math_input(
            "Perspective field of view produces an invalid tangent"));
    }

    const double focal_length = 1.0 / tangent;
    const double aspect = static_cast<double>(aspect_ratio);
    const double near_value = static_cast<double>(near_plane);
    const double far_value = static_cast<double>(far_plane);
    const double depth_range = near_value - far_value;

    const std::array<double, 5U> coefficients{
        focal_length / aspect,
        focal_length,
        far_value / depth_range,
        (far_value * near_value) / depth_range,
        -1.0,
    };
    for (const double coefficient : coefficients) {
        if (!is_representable_float(coefficient)) {
            return Result<Matrix4>::failure(invalid_math_input(
                "Perspective projection exceeds the representable matrix range"));
        }
    }

    Matrix4 matrix;
    matrix.values_[0U] = static_cast<float>(coefficients[0U]);
    matrix.values_[5U] = static_cast<float>(coefficients[1U]);
    matrix.values_[10U] = static_cast<float>(coefficients[2U]);
    matrix.values_[14U] = static_cast<float>(coefficients[3U]);
    matrix.values_[11U] = static_cast<float>(coefficients[4U]);
    return Result<Matrix4>::success(matrix);
}

Result<Matrix4> Matrix4::orthographic_rh_zo(
    float left,
    float right,
    float bottom,
    float top,
    float near_plane,
    float far_plane) {
    if (!std::isfinite(left) || !std::isfinite(right) ||
        !std::isfinite(bottom) || !std::isfinite(top) ||
        !std::isfinite(near_plane) || !std::isfinite(far_plane) ||
        right <= left || top <= bottom ||
        near_plane <= 0.0F || far_plane <= near_plane) {
        return Result<Matrix4>::failure(invalid_math_input(
            "Orthographic projection requires ordered finite bounds and 0 < near < far"));
    }

    const double horizontal_range =
        static_cast<double>(right) - static_cast<double>(left);
    const double vertical_range =
        static_cast<double>(top) - static_cast<double>(bottom);
    const double depth_range =
        static_cast<double>(near_plane) - static_cast<double>(far_plane);

    const std::array<double, 6U> coefficients{
        2.0 / horizontal_range,
        2.0 / vertical_range,
        1.0 / depth_range,
        -(static_cast<double>(right) + static_cast<double>(left)) /
            horizontal_range,
        -(static_cast<double>(top) + static_cast<double>(bottom)) /
            vertical_range,
        static_cast<double>(near_plane) / depth_range,
    };
    for (const double coefficient : coefficients) {
        if (!is_representable_float(coefficient)) {
            return Result<Matrix4>::failure(invalid_math_input(
                "Orthographic projection exceeds the representable matrix range"));
        }
    }

    Matrix4 matrix = Matrix4::identity();
    matrix.values_[0U] = static_cast<float>(coefficients[0U]);
    matrix.values_[5U] = static_cast<float>(coefficients[1U]);
    matrix.values_[10U] = static_cast<float>(coefficients[2U]);
    matrix.values_[12U] = static_cast<float>(coefficients[3U]);
    matrix.values_[13U] = static_cast<float>(coefficients[4U]);
    matrix.values_[14U] = static_cast<float>(coefficients[5U]);
    return Result<Matrix4>::success(matrix);
}

Result<Matrix4> Matrix4::look_at_rh(
    Vector3 eye,
    Vector3 target,
    Vector3 up) {
    if (!eye.is_finite() || !target.is_finite() || !up.is_finite()) {
        return Result<Matrix4>::failure(invalid_math_input(
            "Look-at eye, target, and up vectors must be finite"));
    }

    const DoubleVector3 eye_value = to_double(eye);
    const DoubleVector3 target_value = to_double(target);
    const DoubleVector3 up_value = to_double(up);
    const auto backward = Storm::Math::normalized(
        subtract_double(eye_value, target_value),
        "Look-at eye and target must not coincide");
    if (!backward) {
        return Result<Matrix4>::failure(backward.error());
    }

    const auto right = Storm::Math::normalized(
        cross_double(up_value, backward.value()),
        "Look-at up vector must not be parallel to the view direction");
    if (!right) {
        return Result<Matrix4>::failure(right.error());
    }

    const DoubleVector3 corrected_up = cross_double(backward.value(), right.value());
    const DoubleVector3 x_axis = right.value();
    const DoubleVector3 y_axis = corrected_up;
    const DoubleVector3 z_axis = backward.value();

    const std::array<double, 12U> coefficients{
        x_axis.x, x_axis.y, x_axis.z, -dot_double(x_axis, eye_value),
        y_axis.x, y_axis.y, y_axis.z, -dot_double(y_axis, eye_value),
        z_axis.x, z_axis.y, z_axis.z, -dot_double(z_axis, eye_value),
    };
    for (const double coefficient : coefficients) {
        if (!is_representable_float(coefficient)) {
            return Result<Matrix4>::failure(invalid_math_input(
                "Look-at view matrix exceeds the representable matrix range"));
        }
    }

    Matrix4 matrix = Matrix4::identity();
    // Rows contain the camera basis; storage remains column-major.
    matrix.values_[0U] = static_cast<float>(coefficients[0U]);
    matrix.values_[4U] = static_cast<float>(coefficients[1U]);
    matrix.values_[8U] = static_cast<float>(coefficients[2U]);
    matrix.values_[12U] = static_cast<float>(coefficients[3U]);
    matrix.values_[1U] = static_cast<float>(coefficients[4U]);
    matrix.values_[5U] = static_cast<float>(coefficients[5U]);
    matrix.values_[9U] = static_cast<float>(coefficients[6U]);
    matrix.values_[13U] = static_cast<float>(coefficients[7U]);
    matrix.values_[2U] = static_cast<float>(coefficients[8U]);
    matrix.values_[6U] = static_cast<float>(coefficients[9U]);
    matrix.values_[10U] = static_cast<float>(coefficients[10U]);
    matrix.values_[14U] = static_cast<float>(coefficients[11U]);
    return Result<Matrix4>::success(matrix);
}

Result<Vector3> Matrix4::transform_point(Vector3 point) const {
    if (!is_finite() || !point.is_finite()) {
        return Result<Vector3>::failure(invalid_math_input(
            "Matrix4 and point must contain only finite values"));
    }

    const double x = static_cast<double>(point.x);
    const double y = static_cast<double>(point.y);
    const double z = static_cast<double>(point.z);
    const double transformed_x =
        (static_cast<double>(values_[0U]) * x) +
        (static_cast<double>(values_[4U]) * y) +
        (static_cast<double>(values_[8U]) * z) +
        static_cast<double>(values_[12U]);
    const double transformed_y =
        (static_cast<double>(values_[1U]) * x) +
        (static_cast<double>(values_[5U]) * y) +
        (static_cast<double>(values_[9U]) * z) +
        static_cast<double>(values_[13U]);
    const double transformed_z =
        (static_cast<double>(values_[2U]) * x) +
        (static_cast<double>(values_[6U]) * y) +
        (static_cast<double>(values_[10U]) * z) +
        static_cast<double>(values_[14U]);
    const double transformed_w =
        (static_cast<double>(values_[3U]) * x) +
        (static_cast<double>(values_[7U]) * y) +
        (static_cast<double>(values_[11U]) * z) +
        static_cast<double>(values_[15U]);

    if (!std::isfinite(transformed_w) ||
        std::fabs(transformed_w) <= kMinimumHomogeneousMagnitude) {
        return Result<Vector3>::failure(invalid_math_input(
            "Point transformation produced a zero or invalid homogeneous coordinate"));
    }

    return narrow_vector(
        DoubleVector3{
            transformed_x / transformed_w,
            transformed_y / transformed_w,
            transformed_z / transformed_w,
        },
        "Transformed point exceeds the representable Vector3 range");
}

Result<Vector3> Matrix4::transform_direction(Vector3 direction) const {
    if (!is_finite() || !direction.is_finite()) {
        return Result<Vector3>::failure(invalid_math_input(
            "Matrix4 and direction must contain only finite values"));
    }

    const double x = static_cast<double>(direction.x);
    const double y = static_cast<double>(direction.y);
    const double z = static_cast<double>(direction.z);
    return narrow_vector(
        DoubleVector3{
            (static_cast<double>(values_[0U]) * x) +
                (static_cast<double>(values_[4U]) * y) +
                (static_cast<double>(values_[8U]) * z),
            (static_cast<double>(values_[1U]) * x) +
                (static_cast<double>(values_[5U]) * y) +
                (static_cast<double>(values_[9U]) * z),
            (static_cast<double>(values_[2U]) * x) +
                (static_cast<double>(values_[6U]) * y) +
                (static_cast<double>(values_[10U]) * z),
        },
        "Transformed direction exceeds the representable Vector3 range");
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
