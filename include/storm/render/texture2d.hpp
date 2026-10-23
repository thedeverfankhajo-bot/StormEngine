#pragma once

#include "storm/core/status.hpp"
#include "storm/render/software_canvas2d.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace storm::render {

// Bounded, CPU-resident RGBA8 image data for reference rendering and asset-pipeline tests.
// This type owns its pixels; it does not represent a GPU allocation or a file decoder.
class Texture2D final {
public:
    static constexpr std::uint32_t maximum_dimension = SoftwareCanvas2D::maximum_dimension;
    static constexpr std::size_t maximum_pixels = SoftwareCanvas2D::maximum_pixels;

    // Replaces the current image only when dimensions, payload size and allocation are valid.
    // Pixels are copied, stored row-major, and interpreted as straight-alpha RGBA8.
    [[nodiscard]] core::Status set_pixels(std::uint32_t width, std::uint32_t height,
                                          std::span<const Rgba8> pixels);

    void reset() noexcept;

    [[nodiscard]] bool empty() const noexcept { return pixels_.empty(); }
    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t pixel_count() const noexcept { return pixels_.size(); }

    [[nodiscard]] std::span<const Rgba8> pixels() const noexcept {
        return std::span<const Rgba8>{pixels_.data(), pixels_.size()};
    }

private:
    std::uint32_t width_{};
    std::uint32_t height_{};
    std::vector<Rgba8> pixels_{};
};

} // namespace storm::render
