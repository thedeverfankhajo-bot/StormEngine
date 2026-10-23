#include "storm/render/camera2d.hpp"

#include <cmath>
#include <limits>

namespace storm::render {
namespace {

[[nodiscard]] bool to_finite_float(double value, float& output) noexcept {
    constexpr double maximum = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(value) || value > maximum || value < -maximum) {
        return false;
    }

    output = static_cast<float>(value);
    return std::isfinite(output);
}

} // namespace

bool Camera2D::is_valid() const noexcept {
    return center.is_finite() && std::isfinite(rotation_radians) &&
           std::isfinite(view_height) && view_height > 0.0F && std::isfinite(zoom) &&
           zoom > 0.0F && viewport_width > 0U && viewport_height > 0U;
}

std::optional<math::Matrix4> Camera2D::world_to_pixel_matrix() const noexcept {
    if (!is_valid()) {
        return std::nullopt;
    }

    const double pixels_per_world_unit =
        static_cast<double>(viewport_height) * static_cast<double>(zoom) /
        static_cast<double>(view_height);
    float scale{};
    if (!to_finite_float(pixels_per_world_unit, scale) || scale <= 0.0F) {
        return std::nullopt;
    }

    const float half_width = static_cast<float>(viewport_width) * 0.5F;
    const float half_height = static_cast<float>(viewport_height) * 0.5F;
    const math::Matrix4 result =
        math::Matrix4::translation(math::Vec3{half_width, half_height, 0.0F}) *
        math::Matrix4::scale(math::Vec3{scale, -scale, 1.0F}) *
        math::Matrix4::rotation_z(-rotation_radians) *
        math::Matrix4::translation(math::Vec3{-center[0U], -center[1U], 0.0F});

    return result.is_finite() ? std::optional<math::Matrix4>{result} : std::nullopt;
}

std::optional<math::Vec2> Camera2D::world_to_pixel(
    const math::Vec2& world_position) const noexcept {
    if (!world_position.is_finite()) {
        return std::nullopt;
    }

    const auto transform = world_to_pixel_matrix();
    if (!transform.has_value()) {
        return std::nullopt;
    }

    const auto pixel_position =
        transform->transform_point(math::Vec3{world_position[0U], world_position[1U], 0.0F});
    if (!pixel_position.has_value()) {
        return std::nullopt;
    }

    return math::Vec2{(*pixel_position)[0U], (*pixel_position)[1U]};
}

std::optional<math::Vec2> Camera2D::pixel_to_world(
    const math::Vec2& pixel_position) const noexcept {
    if (!is_valid() || !pixel_position.is_finite()) {
        return std::nullopt;
    }

    const double world_units_per_pixel =
        static_cast<double>(view_height) /
        (static_cast<double>(viewport_height) * static_cast<double>(zoom));
    if (!std::isfinite(world_units_per_pixel) || world_units_per_pixel <= 0.0) {
        return std::nullopt;
    }

    const double pixel_delta_x =
        static_cast<double>(pixel_position[0U]) - static_cast<double>(viewport_width) * 0.5;
    const double pixel_delta_y =
        static_cast<double>(pixel_position[1U]) - static_cast<double>(viewport_height) * 0.5;
    const double camera_x = pixel_delta_x * world_units_per_pixel;
    const double camera_y = -pixel_delta_y * world_units_per_pixel;

    const double angle = static_cast<double>(rotation_radians);
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    const double world_x = static_cast<double>(center[0U]) +
                           cosine * camera_x - sine * camera_y;
    const double world_y = static_cast<double>(center[1U]) +
                           sine * camera_x + cosine * camera_y;

    float x{};
    float y{};
    if (!to_finite_float(world_x, x) || !to_finite_float(world_y, y)) {
        return std::nullopt;
    }

    return math::Vec2{x, y};
}

bool Camera2D::set_zoom_at_pixel(float new_zoom,
                                 const math::Vec2& pixel_position) noexcept {
    if (!is_valid() || !std::isfinite(new_zoom) || new_zoom <= 0.0F ||
        !pixel_position.is_finite()) {
        return false;
    }

    const auto anchored_world = pixel_to_world(pixel_position);
    if (!anchored_world.has_value()) {
        return false;
    }

    Camera2D candidate = *this;
    candidate.zoom = new_zoom;
    const auto shifted_world = candidate.pixel_to_world(pixel_position);
    if (!shifted_world.has_value()) {
        return false;
    }

    candidate.center += *anchored_world - *shifted_world;
    if (!candidate.is_valid()) {
        return false;
    }

    *this = candidate;
    return true;
}

} // namespace storm::render
