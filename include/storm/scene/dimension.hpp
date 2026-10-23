#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace storm::scene {

// Dimension is a scene/project capability, not a different engine binary.
enum class Dimension : std::uint8_t {
    One = 1,
    Two = 2,
    Three = 3
};

[[nodiscard]] constexpr bool is_valid_dimension(Dimension dimension) noexcept {
    switch (dimension) {
    case Dimension::One:
    case Dimension::Two:
    case Dimension::Three:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr std::size_t component_count(Dimension dimension) noexcept {
    return is_valid_dimension(dimension) ? static_cast<std::size_t>(dimension) : 0U;
}

[[nodiscard]] constexpr std::string_view dimension_name(Dimension dimension) noexcept {
    switch (dimension) {
    case Dimension::One:
        return "1D";
    case Dimension::Two:
        return "2D";
    case Dimension::Three:
        return "3D";
    }
    return "Unknown";
}

} // namespace storm::scene
