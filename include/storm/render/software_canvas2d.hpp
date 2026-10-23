#pragma once

#include "storm/core/status.hpp"
#include "storm/math/matrix4.hpp"
#include "storm/math/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace storm::render {

class Texture2D;

struct Rgba8 final {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};

    [[nodiscard]] friend constexpr bool operator==(const Rgba8&, const Rgba8&) noexcept = default;
};

static_assert(sizeof(Rgba8) == 4U);

class SoftwareCanvas2D final {
public:
    static constexpr std::uint32_t maximum_dimension = 8192U;
    static constexpr std::size_t maximum_pixels = 16'777'216U; // 64 MiB at RGBA8.

    [[nodiscard]] core::Status resize(std::uint32_t width, std::uint32_t height);

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t pixel_count() const noexcept { return pixels_.size(); }

    [[nodiscard]] std::span<const Rgba8> pixels() const noexcept {
        return std::span<const Rgba8>{pixels_.data(), pixels_.size()};
    }

    void clear(Rgba8 color) noexcept;

    // Canvas coordinates use a top-left origin and +Y down. The matrix maps a local
    // [0, size.x] x [0, size.y] quad into canvas pixel coordinates.
    // Negative sizes and invalid transforms fail without changing pixels; off-canvas geometry
    // is safely clipped. This bounded CPU rasterizer is a correctness baseline, not SIMD-optimized.
    [[nodiscard]] core::Status draw_rect(const math::Matrix4& local_to_pixel,
                                         const math::Vec2& size,
                                         Rgba8 color);

    // Nearest-neighbor sample of a CPU Texture2D into an affine 2D quad. The matrix maps
    // local quad coordinates to pixels; texture source is row-major with a top-left origin.
    // Invalid/empty textures, non-affine transforms, and negative sizes fail before drawing.
    [[nodiscard]] core::Status draw_texture(
        const math::Matrix4& local_to_pixel, const math::Vec2& size,
        const Texture2D& texture, Rgba8 tint = Rgba8{255U, 255U, 255U, 255U});

private:
    [[nodiscard]] static Rgba8 blend_over(Rgba8 source, Rgba8 destination) noexcept;

    std::uint32_t width_{};
    std::uint32_t height_{};
    std::vector<Rgba8> pixels_{};
};

} // namespace storm::render
