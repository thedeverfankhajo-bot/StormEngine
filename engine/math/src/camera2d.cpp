#include "storm/math/transform2d.hpp"

#include <cmath>
#include <limits>

namespace Storm::Math {
namespace {
[[nodiscard]] Error invalid_camera(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}
[[nodiscard]] bool fits_float(double value) noexcept {
    constexpr double maximum = static_cast<double>(std::numeric_limits<float>::max());
    return std::isfinite(value) && value <= maximum && value >= -maximum;
}
} // namespace

bool Camera2D::is_valid() const noexcept {
    return position.is_finite() && std::isfinite(rotation_radians) &&
        viewport_size.is_finite() && viewport_size.x > 0.0F && viewport_size.y > 0.0F &&
        std::isfinite(zoom) && zoom > 0.0F;
}

Result<Matrix3> Camera2D::view_matrix() const {
    if (!is_valid()) return Result<Matrix3>::failure(invalid_camera("Camera2D requires finite position/rotation, positive viewport dimensions, and positive zoom"));
    const double angle = static_cast<double>(rotation_radians);
    const double cosine = std::cos(angle), sine = std::sin(angle);
    const double px = static_cast<double>(position.x), py = static_cast<double>(position.y);
    const double tx = -((cosine * px) + (sine * py));
    const double ty = (sine * px) - (cosine * py);
    const double values[9]{cosine, -sine, 0.0, sine, cosine, 0.0, tx, ty, 1.0};
    Matrix3 result;
    for (std::size_t index = 0U; index < 9U; ++index) {
        if (!fits_float(values[index])) return Result<Matrix3>::failure(invalid_camera("Camera2D view matrix exceeds the finite float range"));
        result.values_[index] = static_cast<float>(values[index]);
    }
    return Result<Matrix3>::success(result);
}

Result<Matrix3> Camera2D::projection_matrix() const {
    if (!is_valid()) return Result<Matrix3>::failure(invalid_camera("Camera2D requires finite position/rotation, positive viewport dimensions, and positive zoom"));
    const double x_scale = (2.0 * static_cast<double>(zoom)) / static_cast<double>(viewport_size.x);
    const double y_scale = (2.0 * static_cast<double>(zoom)) / static_cast<double>(viewport_size.y);
    if (!fits_float(x_scale) || !fits_float(y_scale)) return Result<Matrix3>::failure(invalid_camera("Camera2D projection exceeds the finite float range"));
    Matrix3 result = Matrix3::identity();
    result.values_[0U] = static_cast<float>(x_scale);
    result.values_[4U] = static_cast<float>(y_scale);
    return Result<Matrix3>::success(result);
}

Result<Matrix3> Camera2D::view_projection() const {
    const auto view = view_matrix();
    if (!view) return Result<Matrix3>::failure(view.error());
    const auto projection = projection_matrix();
    if (!projection) return Result<Matrix3>::failure(projection.error());
    return Matrix3::multiply_checked(projection.value(), view.value());
}

Result<Vector2> Camera2D::world_to_ndc(Vector2 world_point) const {
    const auto combined = view_projection();
    if (!combined) return Result<Vector2>::failure(combined.error());
    return combined.value().transform_point(world_point);
}

} // namespace Storm::Math
