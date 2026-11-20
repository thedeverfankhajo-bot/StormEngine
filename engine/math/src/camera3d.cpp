#include "storm/math/transform3d.hpp"

#include <cmath>

namespace Storm::Math {
namespace {

constexpr double kPi = 3.14159265358979323846264338327950288;
constexpr double kMinimumMagnitudeSquared = 1.0e-24;

[[nodiscard]] Error invalid_camera(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

} // namespace

bool Camera3D::is_valid() const noexcept {
    if (!position.is_finite() || !target.is_finite() || !up.is_finite() ||
        !std::isfinite(near_plane) || !std::isfinite(far_plane) ||
        near_plane <= 0.0F || far_plane <= near_plane) {
        return false;
    }

    const double backward_x =
        static_cast<double>(position.x) - static_cast<double>(target.x);
    const double backward_y =
        static_cast<double>(position.y) - static_cast<double>(target.y);
    const double backward_z =
        static_cast<double>(position.z) - static_cast<double>(target.z);
    const double backward_length_squared =
        (backward_x * backward_x) +
        (backward_y * backward_y) +
        (backward_z * backward_z);
    if (!std::isfinite(backward_length_squared) ||
        backward_length_squared <= kMinimumMagnitudeSquared) {
        return false;
    }

    const double right_x =
        (static_cast<double>(up.y) * backward_z) -
        (static_cast<double>(up.z) * backward_y);
    const double right_y =
        (static_cast<double>(up.z) * backward_x) -
        (static_cast<double>(up.x) * backward_z);
    const double right_z =
        (static_cast<double>(up.x) * backward_y) -
        (static_cast<double>(up.y) * backward_x);
    const double right_length_squared =
        (right_x * right_x) + (right_y * right_y) + (right_z * right_z);
    if (!std::isfinite(right_length_squared) ||
        right_length_squared <= kMinimumMagnitudeSquared) {
        return false;
    }

    switch (projection) {
    case Camera3DProjection::Perspective:
        return std::isfinite(vertical_fov_radians) &&
            static_cast<double>(vertical_fov_radians) > 0.0 &&
            static_cast<double>(vertical_fov_radians) < kPi &&
            std::isfinite(aspect_ratio) && aspect_ratio > 0.0F;

    case Camera3DProjection::Orthographic:
        return std::isfinite(left) && std::isfinite(right) &&
            std::isfinite(bottom) && std::isfinite(top) &&
            right > left && top > bottom;
    }

    return false;
}

Result<Matrix4> Camera3D::view_matrix() const {
    if (!is_valid()) {
        return Result<Matrix4>::failure(invalid_camera(
            "Camera3D pose or projection settings are invalid"));
    }
    return Matrix4::look_at_rh(position, target, up);
}

Result<Matrix4> Camera3D::projection_matrix() const {
    if (!is_valid()) {
        return Result<Matrix4>::failure(invalid_camera(
            "Camera3D pose or projection settings are invalid"));
    }

    switch (projection) {
    case Camera3DProjection::Perspective:
        return Matrix4::perspective_rh_zo(
            vertical_fov_radians, aspect_ratio, near_plane, far_plane);

    case Camera3DProjection::Orthographic:
        return Matrix4::orthographic_rh_zo(
            left, right, bottom, top, near_plane, far_plane);
    }

    return Result<Matrix4>::failure(invalid_camera(
        "Camera3D projection type is not supported"));
}

Result<Matrix4> Camera3D::view_projection_matrix() const {
    const auto view = view_matrix();
    if (!view) return Result<Matrix4>::failure(view.error());
    const auto projection_result = projection_matrix();
    if (!projection_result) return Result<Matrix4>::failure(projection_result.error());
    return Matrix4::multiply_checked(projection_result.value(), view.value());
}

Result<Vector3> Camera3D::world_to_ndc(Vector3 world_position) const {
    const auto combined = view_projection_matrix();
    if (!combined) return Result<Vector3>::failure(combined.error());
    return combined.value().transform_point(world_position);
}

} // namespace Storm::Math
