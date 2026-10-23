#pragma once

#include "storm/math/matrix4.hpp"
#include "storm/math/vector.hpp"

#include <cstdint>
#include <optional>

namespace storm::render {

// Orthographic 2D camera mapping world coordinates to top-left-origin framebuffer pixels.
// World +Y points up; framebuffer +Y points down. Positive camera rotation is counter-clockwise
// in world space. view_height is the visible world-space height at zoom == 1; zoom > 1 zooms in.
// A viewport with either dimension zero is invalid (for example, while minimized).
class Camera2D final {
public:
    math::Vec2 center{};
    float rotation_radians{};
    float view_height{10.0F};
    float zoom{1.0F};
    std::uint32_t viewport_width{};
    std::uint32_t viewport_height{};

    [[nodiscard]] bool is_valid() const noexcept;

    // Returns the affine world-to-pixel matrix suitable for composing with local transforms before
    // drawing to SoftwareCanvas2D. Returns no value for invalid parameters or unrepresentable
    // floating-point results.
    [[nodiscard]] std::optional<math::Matrix4> world_to_pixel_matrix() const noexcept;

    // Continuous pixel coordinates use the framebuffer's top-left corner as (0, 0); no rounding
    // or clipping is performed. Conversion can fail for invalid parameters or numerical overflow.
    [[nodiscard]] std::optional<math::Vec2> world_to_pixel(
        const math::Vec2& world_position) const noexcept;
    [[nodiscard]] std::optional<math::Vec2> pixel_to_world(
        const math::Vec2& pixel_position) const noexcept;
};

} // namespace storm::render
