#pragma once

#include "storm/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Storm::Serialization {

// Bounded little-endian writer used by versioned engine archives.
class BinaryWriter final {
public:
    explicit BinaryWriter(std::size_t maximum_size);

    [[nodiscard]] Result<void> write_u8(std::uint8_t value);
    [[nodiscard]] Result<void> write_u16(std::uint16_t value);
    [[nodiscard]] Result<void> write_u32(std::uint32_t value);
    [[nodiscard]] Result<void> write_f32(float value);
    [[nodiscard]] Result<void> write_bytes(std::span<const std::byte> value);
    [[nodiscard]] Result<void> write_string(
        std::string_view value,
        std::size_t maximum_string_size);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::vector<std::byte> take() && noexcept;

private:
    std::size_t maximum_size_{0U};
    std::vector<std::byte> bytes_;
};

// Bounds-checked little-endian reader. It never advances on a failed read.
class BinaryReader final {
public:
    explicit BinaryReader(std::span<const std::byte> bytes) noexcept;

    [[nodiscard]] Result<std::uint8_t> read_u8();
    [[nodiscard]] Result<std::uint16_t> read_u16();
    [[nodiscard]] Result<std::uint32_t> read_u32();
    [[nodiscard]] Result<float> read_f32();
    [[nodiscard]] Result<std::span<const std::byte>> read_bytes(std::size_t count);
    [[nodiscard]] Result<std::string> read_string(std::size_t maximum_string_size);

    [[nodiscard]] std::size_t remaining() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{0U};
};

} // namespace Storm::Serialization
