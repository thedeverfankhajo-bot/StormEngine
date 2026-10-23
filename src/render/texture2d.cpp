#include "storm/render/texture2d.hpp"

#include <new>
#include <string>
#include <vector>

namespace storm::render {
namespace {

[[nodiscard]] core::Status invalid_texture_status(const char* message) {
    return core::Status::failure(core::ErrorCode::InvalidArgument, message);
}

[[nodiscard]] core::Status exhausted_texture_status() {
    return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
}

} // namespace

core::Status Texture2D::set_pixels(std::uint32_t width, std::uint32_t height,
                                   std::span<const Rgba8> pixels) {
    if (width == 0U || height == 0U) {
        return invalid_texture_status("texture dimensions must be non-zero");
    }
    if (width > maximum_dimension || height > maximum_dimension) {
        return invalid_texture_status("texture dimension exceeds its limit");
    }

    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixel_count > maximum_pixels) {
        return core::Status::failure(
            core::ErrorCode::ResourceExhausted, "texture exceeds its pixel budget");
    }
    if (pixels.size() != pixel_count) {
        return invalid_texture_status("texture payload size does not match its dimensions");
    }

    try {
        std::vector<Rgba8> replacement(pixels.begin(), pixels.end());
        pixels_.swap(replacement);
        width_ = width;
        height_ = height;
    } catch (const std::bad_alloc&) {
        return exhausted_texture_status();
    }

    return core::Status::success();
}

void Texture2D::reset() noexcept {
    std::vector<Rgba8>{}.swap(pixels_);
    width_ = 0U;
    height_ = 0U;
}

} // namespace storm::render
