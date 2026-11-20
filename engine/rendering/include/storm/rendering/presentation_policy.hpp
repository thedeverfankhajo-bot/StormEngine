#pragma once

#include "storm/foundation/result.hpp"

#include <cstdint>

namespace Storm::Rendering {

struct PresentationExtent final {
    std::uint32_t width{0U};
    std::uint32_t height{0U};

    [[nodiscard]] friend bool operator==(
        const PresentationExtent&, const PresentationExtent&) noexcept = default;
};

// A current extent with both dimensions set to UINT32_MAX means the platform
// lets the application choose an extent within the supplied bounds.
[[nodiscard]] Result<PresentationExtent> resolve_presentation_extent(
    PresentationExtent requested,
    PresentationExtent current,
    PresentationExtent minimum,
    PresentationExtent maximum);

// max_image_count == 0 follows the Vulkan convention for an unbounded maximum.
// The result never exceeds hard_limit and always respects the surface minimum.
[[nodiscard]] Result<std::uint32_t> choose_presentation_image_count(
    std::uint32_t min_image_count,
    std::uint32_t max_image_count,
    std::uint32_t hard_limit);

} // namespace Storm::Rendering
