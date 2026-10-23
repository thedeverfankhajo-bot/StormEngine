#pragma once

#include "storm/math/vector.hpp"

#include <array>
#include <cstdint>

namespace storm::scene {

// CPU-reference-friendly 2D quad metadata. This is scene data, not a GPU resource/material.
// Texture, material, layer ordering and renderer-resource handles remain separate future contracts.
struct Renderable2D final {
    math::Vec2 size{1.0F, 1.0F};
    std::array<std::uint8_t, 4U> tint_rgba{255U, 255U, 255U, 255U};
    bool visible{true};

    [[nodiscard]] bool is_valid() const noexcept {
        return size.is_finite() && size[0U] > 0.0F && size[1U] > 0.0F;
    }
};

} // namespace storm::scene
