#include <storm/core/version.hpp>
#include <storm/math/projection.hpp>
#include <storm/math/quaternion.hpp>
#include <storm/math/vector.hpp>

#include <cmath>
#include <string_view>

int main() {
    const storm::math::Vec2 sample{3.0F, 4.0F};
    constexpr float half_pi = 1.57079632679489661923F;
    constexpr float tolerance = 1.0e-5F;

    const auto rotation = storm::math::Quaternion::from_axis_angle(
        storm::math::Vec3{0.0F, 0.0F, 1.0F}, half_pi);
    if (!rotation.has_value()) {
        return 2;
    }
    const auto rotated = rotation->rotate_vector(storm::math::Vec3{1.0F, 0.0F, 0.0F});
    if (!rotated.has_value()) {
        return 3;
    }
    const bool rotation_ok = std::abs((*rotated)[0U]) < tolerance &&
                             std::abs((*rotated)[1U] - 1.0F) < tolerance &&
                             std::abs((*rotated)[2U]) < tolerance;

    const auto projection = storm::math::Projection::perspective_rh(
        half_pi, 16.0F / 9.0F, 0.1F, 100.0F, storm::math::ClipDepthRange::ZeroToOne);
    if (!projection.has_value()) {
        return 4;
    }
    const auto projected_near = projection->transform_point(storm::math::Vec3{0.0F, 0.0F, -0.1F});
    const auto projected_far = projection->transform_point(storm::math::Vec3{0.0F, 0.0F, -100.0F});
    if (!projected_near.has_value() || !projected_far.has_value()) {
        return 5;
    }
    const bool depth_ok = std::abs((*projected_near)[2U]) < tolerance &&
                          std::abs((*projected_far)[2U] - 1.0F) < tolerance;

    return storm::version_string() == std::string_view{"0.1.0"} &&
                   sample.length_squared() == 25.0F && rotation_ok && depth_ok
               ? 0
               : 1;
}
