#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace storm::core {

// A persistent 128-bit identifier value. This type parses/formats UUID text but does not
// generate identifiers; generation requires an explicitly chosen entropy or content policy.
class StableId final {
public:
    using Bytes = std::array<std::uint8_t, 16>;

    static constexpr std::size_t byte_count = 16U;
    static constexpr std::size_t text_length = 36U;

    constexpr StableId() noexcept = default;

    [[nodiscard]] static constexpr std::optional<StableId> from_bytes(const Bytes& bytes) noexcept {
        if (is_nil(bytes)) {
            return std::nullopt;
        }
        return StableId{bytes};
    }

    // Parses the canonical UUID 8-4-4-4-12 hex-and-dash form. Hex may be upper or lower case.
    // The all-zero Nil UUID is rejected because it represents absence, not a stable object ID.
    [[nodiscard]] static constexpr std::optional<StableId> parse(std::string_view text) noexcept {
        if (text.size() != text_length) {
            return std::nullopt;
        }

        Bytes bytes{};
        std::size_t output_index = 0U;
        int high_nibble = -1;

        for (std::size_t index = 0U; index < text.size(); ++index) {
            const bool is_separator =
                index == 8U || index == 13U || index == 18U || index == 23U;
            if (is_separator) {
                if (text[index] != '-') {
                    return std::nullopt;
                }
                continue;
            }

            const int nibble = hex_value(text[index]);
            if (nibble < 0) {
                return std::nullopt;
            }

            if (high_nibble < 0) {
                high_nibble = nibble;
            } else {
                if (output_index >= bytes.size()) {
                    return std::nullopt;
                }
                bytes[output_index] = static_cast<std::uint8_t>(
                    (static_cast<unsigned int>(high_nibble) << 4U) |
                    static_cast<unsigned int>(nibble));
                ++output_index;
                high_nibble = -1;
            }
        }

        if (output_index != bytes.size() || high_nibble >= 0 || is_nil(bytes)) {
            return std::nullopt;
        }
        return StableId{bytes};
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return !is_nil(bytes_);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return is_valid();
    }

    // Bytes use network/text order; do not reinterpret them as host-endian integers.
    [[nodiscard]] constexpr const Bytes& bytes() const noexcept {
        return bytes_;
    }

    // Returns lowercase canonical UUID text, or an empty string for an invalid/default ID.
    [[nodiscard]] std::string to_string() const {
        if (!is_valid()) {
            return {};
        }

        constexpr char digits[] = "0123456789abcdef";
        std::string text;
        text.reserve(text_length);

        for (std::size_t index = 0U; index < bytes_.size(); ++index) {
            if (index == 4U || index == 6U || index == 8U || index == 10U) {
                text.push_back('-');
            }
            const auto value = static_cast<unsigned int>(bytes_[index]);
            text.push_back(digits[(value >> 4U) & 0x0FU]);
            text.push_back(digits[value & 0x0FU]);
        }
        return text;
    }

    [[nodiscard]] friend constexpr bool operator==(const StableId&, const StableId&) noexcept =
        default;

private:
    explicit constexpr StableId(const Bytes& bytes) noexcept : bytes_(bytes) {}

    [[nodiscard]] static constexpr bool is_nil(const Bytes& bytes) noexcept {
        for (const std::uint8_t value : bytes) {
            if (value != 0U) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr int hex_value(char character) noexcept {
        if (character >= '0' && character <= '9') {
            return character - '0';
        }
        if (character >= 'a' && character <= 'f') {
            return character - 'a' + 10;
        }
        if (character >= 'A' && character <= 'F') {
            return character - 'A' + 10;
        }
        return -1;
    }

    Bytes bytes_{};
};


// Deterministic bytewise ordering shared by scene ownership and ID registries.
// Byte order follows the canonical UUID representation, not host-endian integer order.
struct StableIdLess final {
    [[nodiscard]] bool operator()(const StableId& left, const StableId& right) const noexcept {
        const auto& left_bytes = left.bytes();
        const auto& right_bytes = right.bytes();

        for (std::size_t index = 0U; index < StableId::byte_count; ++index) {
            if (left_bytes[index] != right_bytes[index]) {
                return left_bytes[index] < right_bytes[index];
            }
        }
        return false;
    }
};

} // namespace storm::core
