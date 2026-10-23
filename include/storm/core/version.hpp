#pragma once

#include <cstdint>
#include <string_view>

namespace storm {

struct Version final {
    std::uint32_t major{};
    std::uint32_t minor{};
    std::uint32_t patch{};

    [[nodiscard]] friend constexpr bool operator==(const Version&, const Version&) noexcept = default;
};

inline constexpr Version kVersion{0, 1, 0};

[[nodiscard]] Version version() noexcept;
[[nodiscard]] std::string_view version_string() noexcept;

} // namespace storm
