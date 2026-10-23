#include "storm/render/software_canvas2d.hpp"
#include "storm/render/texture2d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>

namespace storm::render {
namespace {

[[nodiscard]] core::Status invalid_draw_status(const char* message) {
    return core::Status::failure(core::ErrorCode::InvalidArgument, message);
}

[[nodiscard]] core::Status exhausted_status() {
    return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
}

[[nodiscard]] double cross(const math::Vec2& a, const math::Vec2& b,
                           double point_x, double point_y) noexcept {
    const double ax = static_cast<double>(a[0U]);
    const double ay = static_cast<double>(a[1U]);
    const double bx = static_cast<double>(b[0U]);
    const double by = static_cast<double>(b[1U]);
    return (bx - ax) * (point_y - ay) - (by - ay) * (point_x - ax);
}

} // namespace

core::Status SoftwareCanvas2D::resize(std::uint32_t width, std::uint32_t height) {
    if (width > maximum_dimension || height > maximum_dimension) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument, "software canvas dimension exceeds its limit");
    }

    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixel_count > maximum_pixels) {
        return core::Status::failure(
            core::ErrorCode::ResourceExhausted, "software canvas exceeds its pixel budget");
    }

    try {
        std::vector<Rgba8> replacement(pixel_count);
        pixels_.swap(replacement);
        width_ = width;
        height_ = height;
    } catch (const std::bad_alloc&) {
        return exhausted_status();
    }

    return core::Status::success();
}

void SoftwareCanvas2D::clear(Rgba8 color) noexcept {
    std::fill(pixels_.begin(), pixels_.end(), color);
}

core::Status SoftwareCanvas2D::draw_rect(const math::Matrix4& local_to_pixel,
                                         const math::Vec2& size,
                                         Rgba8 color) {
    if (!local_to_pixel.is_finite() || !size.is_finite() ||
        size[0U] < 0.0F || size[1U] < 0.0F) {
        return invalid_draw_status("software rectangle has an invalid transform or size");
    }

    if (pixels_.empty() || size[0U] == 0.0F || size[1U] == 0.0F || color.a == 0U) {
        return core::Status::success();
    }

    const std::array<math::Vec2, 4U> local_corners{
        math::Vec2{0.0F, 0.0F},
        math::Vec2{size[0U], 0.0F},
        math::Vec2{size[0U], size[1U]},
        math::Vec2{0.0F, size[1U]}
    };
    std::array<math::Vec2, 4U> corners{};
    for (std::size_t index = 0U; index < local_corners.size(); ++index) {
        const auto transformed = local_to_pixel.transform_point(
            math::Vec3{local_corners[index][0U], local_corners[index][1U], 0.0F});
        if (!transformed.has_value()) {
            return invalid_draw_status("software rectangle transform produced an invalid vertex");
        }
        corners[index] = math::Vec2{(*transformed)[0U], (*transformed)[1U]};
    }

    double signed_area_twice = 0.0;
    double minimum_x = static_cast<double>(corners[0U][0U]);
    double maximum_x = minimum_x;
    double minimum_y = static_cast<double>(corners[0U][1U]);
    double maximum_y = minimum_y;
    for (std::size_t index = 0U; index < corners.size(); ++index) {
        const math::Vec2& current = corners[index];
        const math::Vec2& next = corners[(index + 1U) % corners.size()];
        const double current_x = static_cast<double>(current[0U]);
        const double current_y = static_cast<double>(current[1U]);
        const double next_x = static_cast<double>(next[0U]);
        const double next_y = static_cast<double>(next[1U]);
        signed_area_twice += current_x * next_y - next_x * current_y;
        minimum_x = std::min(minimum_x, current_x);
        maximum_x = std::max(maximum_x, current_x);
        minimum_y = std::min(minimum_y, current_y);
        maximum_y = std::max(maximum_y, current_y);
    }

    constexpr double area_epsilon = 1.0e-12;
    if (std::abs(signed_area_twice) <= area_epsilon) {
        return core::Status::success();
    }

    const double canvas_width = static_cast<double>(width_);
    const double canvas_height = static_cast<double>(height_);
    if (maximum_x <= 0.0 || maximum_y <= 0.0 ||
        minimum_x >= canvas_width || minimum_y >= canvas_height) {
        return core::Status::success();
    }

    const double clipped_left = std::clamp(minimum_x, 0.0, canvas_width);
    const double clipped_right = std::clamp(maximum_x, 0.0, canvas_width);
    const double clipped_top = std::clamp(minimum_y, 0.0, canvas_height);
    const double clipped_bottom = std::clamp(maximum_y, 0.0, canvas_height);
    const auto x_begin = static_cast<std::uint32_t>(std::floor(clipped_left));
    const auto x_end = static_cast<std::uint32_t>(std::ceil(clipped_right));
    const auto y_begin = static_cast<std::uint32_t>(std::floor(clipped_top));
    const auto y_end = static_cast<std::uint32_t>(std::ceil(clipped_bottom));
    const bool positive_orientation = signed_area_twice > 0.0;
    constexpr double edge_epsilon = 1.0e-9;

    for (std::uint32_t y = y_begin; y < y_end; ++y) {
        const double sample_y = static_cast<double>(y) + 0.5;
        for (std::uint32_t x = x_begin; x < x_end; ++x) {
            const double sample_x = static_cast<double>(x) + 0.5;
            bool inside = true;
            for (std::size_t edge = 0U; edge < corners.size(); ++edge) {
                const double edge_value =
                    cross(corners[edge], corners[(edge + 1U) % corners.size()],
                          sample_x, sample_y);
                if ((positive_orientation && edge_value < -edge_epsilon) ||
                    (!positive_orientation && edge_value > edge_epsilon)) {
                    inside = false;
                    break;
                }
            }

            if (inside) {
                const std::size_t offset =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                    static_cast<std::size_t>(x);
                pixels_[offset] = blend_over(color, pixels_[offset]);
            }
        }
    }

    return core::Status::success();
}
core::Status SoftwareCanvas2D::draw_texture(const math::Matrix4& local_to_pixel,
                                            const math::Vec2& size,
                                            const Texture2D& texture, Rgba8 tint) {
    if (!local_to_pixel.is_finite() || !size.is_finite() ||
        size[0U] < 0.0F || size[1U] < 0.0F) {
        return invalid_draw_status("software texture has an invalid transform or size");
    }
    if (texture.empty() || texture.width() == 0U || texture.height() == 0U ||
        texture.pixel_count() !=
            static_cast<std::size_t>(texture.width()) * static_cast<std::size_t>(texture.height())) {
        return invalid_draw_status("software texture has no valid pixel data");
    }

    // Texture interpolation is defined for affine 2D transforms only; perspective projection
    // belongs in a future GPU renderer. Keep the test strict so UV coordinates stay predictable.
    if (local_to_pixel(0U, 2U) != 0.0F || local_to_pixel(1U, 2U) != 0.0F ||
        local_to_pixel(2U, 0U) != 0.0F || local_to_pixel(2U, 1U) != 0.0F ||
        local_to_pixel(2U, 2U) != 1.0F ||
        local_to_pixel(3U, 0U) != 0.0F || local_to_pixel(3U, 1U) != 0.0F ||
        local_to_pixel(3U, 2U) != 0.0F || local_to_pixel(3U, 3U) != 1.0F) {
        return invalid_draw_status("software texture requires an affine 2D transform");
    }

    if (pixels_.empty() || size[0U] == 0.0F || size[1U] == 0.0F || tint.a == 0U) {
        return core::Status::success();
    }

    const std::array<math::Vec2, 4U> local_corners{
        math::Vec2{0.0F, 0.0F},
        math::Vec2{size[0U], 0.0F},
        math::Vec2{size[0U], size[1U]},
        math::Vec2{0.0F, size[1U]}
    };
    std::array<math::Vec2, 4U> corners{};
    for (std::size_t index = 0U; index < local_corners.size(); ++index) {
        const auto transformed = local_to_pixel.transform_point(
            math::Vec3{local_corners[index][0U], local_corners[index][1U], 0.0F});
        if (!transformed.has_value()) {
            return invalid_draw_status("software texture transform produced an invalid vertex");
        }
        corners[index] = math::Vec2{(*transformed)[0U], (*transformed)[1U]};
    }

    double signed_area_twice = 0.0;
    double minimum_x = static_cast<double>(corners[0U][0U]);
    double maximum_x = minimum_x;
    double minimum_y = static_cast<double>(corners[0U][1U]);
    double maximum_y = minimum_y;
    for (std::size_t index = 0U; index < corners.size(); ++index) {
        const math::Vec2& current = corners[index];
        const math::Vec2& next = corners[(index + 1U) % corners.size()];
        const double current_x = static_cast<double>(current[0U]);
        const double current_y = static_cast<double>(current[1U]);
        const double next_x = static_cast<double>(next[0U]);
        const double next_y = static_cast<double>(next[1U]);
        signed_area_twice += current_x * next_y - next_x * current_y;
        minimum_x = std::min(minimum_x, current_x);
        maximum_x = std::max(maximum_x, current_x);
        minimum_y = std::min(minimum_y, current_y);
        maximum_y = std::max(maximum_y, current_y);
    }

    constexpr double area_epsilon = 1.0e-12;
    if (std::abs(signed_area_twice) <= area_epsilon) {
        return core::Status::success();
    }

    const double canvas_width = static_cast<double>(width_);
    const double canvas_height = static_cast<double>(height_);
    if (maximum_x <= 0.0 || maximum_y <= 0.0 ||
        minimum_x >= canvas_width || minimum_y >= canvas_height) {
        return core::Status::success();
    }

    const double clipped_left = std::clamp(minimum_x, 0.0, canvas_width);
    const double clipped_right = std::clamp(maximum_x, 0.0, canvas_width);
    const double clipped_top = std::clamp(minimum_y, 0.0, canvas_height);
    const double clipped_bottom = std::clamp(maximum_y, 0.0, canvas_height);
    const auto x_begin = static_cast<std::uint32_t>(std::floor(clipped_left));
    const auto x_end = static_cast<std::uint32_t>(std::ceil(clipped_right));
    const auto y_begin = static_cast<std::uint32_t>(std::floor(clipped_top));
    const auto y_end = static_cast<std::uint32_t>(std::ceil(clipped_bottom));
    const bool positive_orientation = signed_area_twice > 0.0;
    constexpr double edge_epsilon = 1.0e-9;

    const double edge_x_x =
        static_cast<double>(corners[1U][0U]) - static_cast<double>(corners[0U][0U]);
    const double edge_x_y =
        static_cast<double>(corners[1U][1U]) - static_cast<double>(corners[0U][1U]);
    const double edge_y_x =
        static_cast<double>(corners[3U][0U]) - static_cast<double>(corners[0U][0U]);
    const double edge_y_y =
        static_cast<double>(corners[3U][1U]) - static_cast<double>(corners[0U][1U]);
    const double determinant = edge_x_x * edge_y_y - edge_x_y * edge_y_x;
    if (!std::isfinite(determinant) || std::abs(determinant) <= area_epsilon) {
        return core::Status::success();
    }

    const std::span<const Rgba8> source = texture.pixels();
    const double texture_width = static_cast<double>(texture.width());
    const double texture_height = static_cast<double>(texture.height());
    for (std::uint32_t y = y_begin; y < y_end; ++y) {
        const double sample_y = static_cast<double>(y) + 0.5;
        for (std::uint32_t x = x_begin; x < x_end; ++x) {
            const double sample_x = static_cast<double>(x) + 0.5;
            bool inside = true;
            for (std::size_t edge = 0U; edge < corners.size(); ++edge) {
                const double edge_value =
                    cross(corners[edge], corners[(edge + 1U) % corners.size()],
                          sample_x, sample_y);
                if ((positive_orientation && edge_value < -edge_epsilon) ||
                    (!positive_orientation && edge_value > edge_epsilon)) {
                    inside = false;
                    break;
                }
            }
            if (!inside) {
                continue;
            }

            const double relative_x = sample_x - static_cast<double>(corners[0U][0U]);
            const double relative_y = sample_y - static_cast<double>(corners[0U][1U]);
            const double u = std::clamp(
                (relative_x * edge_y_y - relative_y * edge_y_x) / determinant, 0.0, 1.0);
            const double v = std::clamp(
                (edge_x_x * relative_y - edge_x_y * relative_x) / determinant, 0.0, 1.0);
            const auto source_x = static_cast<std::uint32_t>(
                std::min(std::floor(u * texture_width), texture_width - 1.0));
            const auto source_y = static_cast<std::uint32_t>(
                std::min(std::floor(v * texture_height), texture_height - 1.0));
            const std::size_t source_offset =
                static_cast<std::size_t>(source_y) * static_cast<std::size_t>(texture.width()) +
                static_cast<std::size_t>(source_x);
            const Rgba8 texel = source[source_offset];
            if (texel.a == 0U) {
                continue;
            }

            const auto tint_channel = [](std::uint8_t color, std::uint8_t factor) noexcept {
                const unsigned int product =
                    static_cast<unsigned int>(color) * static_cast<unsigned int>(factor);
                return static_cast<std::uint8_t>((product + 127U) / 255U);
            };
            const Rgba8 tinted{
                tint_channel(texel.r, tint.r),
                tint_channel(texel.g, tint.g),
                tint_channel(texel.b, tint.b),
                tint_channel(texel.a, tint.a)
            };
            if (tinted.a == 0U) {
                continue;
            }

            const std::size_t destination_offset =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x);
            pixels_[destination_offset] = blend_over(tinted, pixels_[destination_offset]);
        }
    }

    return core::Status::success();
}

Rgba8 SoftwareCanvas2D::blend_over(Rgba8 source, Rgba8 destination) noexcept {
    if (source.a == 255U) {
        return source;
    }
    if (source.a == 0U) {
        return destination;
    }

    const std::uint64_t source_alpha = static_cast<std::uint64_t>(source.a);
    const std::uint64_t destination_alpha = static_cast<std::uint64_t>(destination.a);
    const std::uint64_t inverse_source_alpha = 255U - source_alpha;
    const std::uint64_t output_alpha_numerator =
        source_alpha * 255U + destination_alpha * inverse_source_alpha;
    if (output_alpha_numerator == 0U) {
        return Rgba8{};
    }

    const auto blend_channel = [source_alpha, destination_alpha, inverse_source_alpha,
                                output_alpha_numerator](std::uint8_t source_channel,
                                                        std::uint8_t destination_channel) noexcept {
        const std::uint64_t numerator =
            static_cast<std::uint64_t>(source_channel) * source_alpha * 255U +
            static_cast<std::uint64_t>(destination_channel) * destination_alpha *
                inverse_source_alpha;
        return static_cast<std::uint8_t>(
            (numerator + output_alpha_numerator / 2U) / output_alpha_numerator);
    };

    const auto output_alpha = static_cast<std::uint8_t>(
        (output_alpha_numerator + 127U) / 255U);
    return Rgba8{blend_channel(source.r, destination.r),
                 blend_channel(source.g, destination.g),
                 blend_channel(source.b, destination.b),
                 output_alpha};
}

} // namespace storm::render
