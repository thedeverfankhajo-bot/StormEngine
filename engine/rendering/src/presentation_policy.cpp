#include "storm/rendering/presentation_policy.hpp"

#include <algorithm>
#include <limits>

namespace Storm::Rendering {
namespace {

[[nodiscard]] Error invalid_argument(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

[[nodiscard]] Error resource_exhausted(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

} // namespace

Result<PresentationExtent> resolve_presentation_extent(
    PresentationExtent requested,
    PresentationExtent current,
    PresentationExtent minimum,
    PresentationExtent maximum) {
    constexpr std::uint32_t variable = std::numeric_limits<std::uint32_t>::max();
    const bool width_is_variable = current.width == variable;
    const bool height_is_variable = current.height == variable;

    if (requested.width == 0U || requested.height == 0U ||
        minimum.width == 0U || minimum.height == 0U ||
        maximum.width < minimum.width || maximum.height < minimum.height ||
        width_is_variable != height_is_variable) {
        return Result<PresentationExtent>::failure(invalid_argument(
            "Presentation extent request or bounds are invalid"));
    }

    if (!width_is_variable) {
        if (current.width == 0U || current.height == 0U ||
            current.width < minimum.width || current.width > maximum.width ||
            current.height < minimum.height || current.height > maximum.height) {
            return Result<PresentationExtent>::failure(invalid_argument(
                "Fixed presentation extent is outside the advertised surface bounds"));
        }
        return Result<PresentationExtent>::success(current);
    }

    return Result<PresentationExtent>::success(PresentationExtent{
        std::clamp(requested.width, minimum.width, maximum.width),
        std::clamp(requested.height, minimum.height, maximum.height)});
}

Result<std::uint32_t> choose_presentation_image_count(
    std::uint32_t min_image_count,
    std::uint32_t max_image_count,
    std::uint32_t hard_limit) {
    if (min_image_count == 0U || hard_limit == 0U ||
        (max_image_count != 0U && max_image_count < min_image_count)) {
        return Result<std::uint32_t>::failure(invalid_argument(
            "Presentation image-count limits are invalid"));
    }
    if (min_image_count > hard_limit) {
        return Result<std::uint32_t>::failure(resource_exhausted(
            "Surface minimum image count exceeds the configured mobile limit"));
    }

    // Prefer one image beyond the minimum for producer/consumer overlap while
    // avoiding both integer overflow and unbounded driver-controlled allocation.
    std::uint32_t selected = min_image_count;
    if (selected < hard_limit) {
        ++selected;
    }
    if (max_image_count != 0U) {
        selected = std::min(selected, max_image_count);
    }
    return Result<std::uint32_t>::success(selected);
}

} // namespace Storm::Rendering
