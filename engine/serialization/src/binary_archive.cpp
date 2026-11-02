#include "storm/serialization/binary_archive.hpp"

#include <array>
#include <bit>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Serialization {
namespace {

[[nodiscard]] Error archive_limit_error(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

[[nodiscard]] Error malformed_archive_error(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

} // namespace

BinaryWriter::BinaryWriter(std::size_t maximum_size)
    : maximum_size_(maximum_size) {}

Result<void> BinaryWriter::write_u8(std::uint8_t value) {
    const std::array<std::byte, 1U> bytes{
        static_cast<std::byte>(value),
    };
    return write_bytes(bytes);
}

Result<void> BinaryWriter::write_u16(std::uint16_t value) {
    const std::array<std::byte, 2U> bytes{
        static_cast<std::byte>(value & 0xffU),
        static_cast<std::byte>((value >> 8U) & 0xffU),
    };
    return write_bytes(bytes);
}

Result<void> BinaryWriter::write_u32(std::uint32_t value) {
    const std::array<std::byte, 4U> bytes{
        static_cast<std::byte>(value & 0xffU),
        static_cast<std::byte>((value >> 8U) & 0xffU),
        static_cast<std::byte>((value >> 16U) & 0xffU),
        static_cast<std::byte>((value >> 24U) & 0xffU),
    };
    return write_bytes(bytes);
}

Result<void> BinaryWriter::write_u64(std::uint64_t value) {
    const std::array<std::byte, 8U> bytes{
        static_cast<std::byte>(value & 0xffU),
        static_cast<std::byte>((value >> 8U) & 0xffU),
        static_cast<std::byte>((value >> 16U) & 0xffU),
        static_cast<std::byte>((value >> 24U) & 0xffU),
        static_cast<std::byte>((value >> 32U) & 0xffU),
        static_cast<std::byte>((value >> 40U) & 0xffU),
        static_cast<std::byte>((value >> 48U) & 0xffU),
        static_cast<std::byte>((value >> 56U) & 0xffU),
    };
    return write_bytes(bytes);
}

Result<void> BinaryWriter::write_f32(float value) {
    return write_u32(std::bit_cast<std::uint32_t>(value));
}

Result<void> BinaryWriter::write_f64(double value) {
    return write_u64(std::bit_cast<std::uint64_t>(value));
}

Result<void> BinaryWriter::write_bytes(std::span<const std::byte> value) {
    if (bytes_.size() > maximum_size_ ||
        value.size() > maximum_size_ - bytes_.size()) {
        return Result<void>::failure(archive_limit_error(
            "The encoded archive exceeds its configured byte limit"));
    }

    try {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    } catch (const std::bad_alloc&) {
        return Result<void>::failure(archive_limit_error(
            "The archive writer could not allocate output storage"));
    } catch (const std::length_error&) {
        return Result<void>::failure(archive_limit_error(
            "The archive writer reached the container size limit"));
    }

    return Result<void>::success();
}

Result<void> BinaryWriter::write_string(
    std::string_view value,
    std::size_t maximum_string_size) {
    if (value.size() > maximum_string_size ||
        value.size() > static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max())) {
        return Result<void>::failure(archive_limit_error(
            "A string exceeds its configured archive limit"));
    }

    constexpr std::size_t length_prefix_size = sizeof(std::uint32_t);
    if (bytes_.size() > maximum_size_ ||
        length_prefix_size > maximum_size_ - bytes_.size() ||
        value.size() > maximum_size_ - bytes_.size() - length_prefix_size) {
        return Result<void>::failure(archive_limit_error(
            "The encoded string would exceed the archive byte limit"));
    }

    const std::size_t required_size =
        bytes_.size() + length_prefix_size + value.size();
    try {
        // Reserve the complete field up front so allocation failure cannot
        // leave a length prefix without its corresponding string bytes.
        bytes_.reserve(required_size);
    } catch (const std::bad_alloc&) {
        return Result<void>::failure(archive_limit_error(
            "The archive writer could not reserve string storage"));
    } catch (const std::length_error&) {
        return Result<void>::failure(archive_limit_error(
            "The archive writer reached the container size limit"));
    }

    const Result<void> length_result =
        write_u32(static_cast<std::uint32_t>(value.size()));
    if (!length_result) {
        return length_result;
    }

    if (value.empty()) {
        return Result<void>::success();
    }

    const auto* first = reinterpret_cast<const std::byte*>(value.data());
    return write_bytes(std::span<const std::byte>{first, value.size()});
}

std::size_t BinaryWriter::size() const noexcept {
    return bytes_.size();
}

std::vector<std::byte> BinaryWriter::take() && noexcept {
    return std::move(bytes_);
}

BinaryReader::BinaryReader(std::span<const std::byte> bytes) noexcept
    : bytes_(bytes) {}

Result<std::uint8_t> BinaryReader::read_u8() {
    const Result<std::span<const std::byte>> bytes = read_bytes(1U);
    if (!bytes) {
        return Result<std::uint8_t>::failure(bytes.error());
    }
    return Result<std::uint8_t>::success(
        std::to_integer<std::uint8_t>(bytes.value()[0U]));
}

Result<std::uint16_t> BinaryReader::read_u16() {
    const Result<std::span<const std::byte>> bytes = read_bytes(2U);
    if (!bytes) {
        return Result<std::uint16_t>::failure(bytes.error());
    }

    const std::uint16_t value =
        static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes.value()[0U])) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                std::to_integer<std::uint8_t>(bytes.value()[1U])) << 8U);
    return Result<std::uint16_t>::success(value);
}

Result<std::uint32_t> BinaryReader::read_u32() {
    const Result<std::span<const std::byte>> bytes = read_bytes(4U);
    if (!bytes) {
        return Result<std::uint32_t>::failure(bytes.error());
    }

    std::uint32_t value = 0U;
    for (std::size_t index = 0U; index < 4U; ++index) {
        const auto byte = std::to_integer<std::uint8_t>(bytes.value()[index]);
        value |= static_cast<std::uint32_t>(byte) << (index * 8U);
    }
    return Result<std::uint32_t>::success(value);
}

Result<std::uint64_t> BinaryReader::read_u64() {
    const Result<std::span<const std::byte>> bytes = read_bytes(8U);
    if (!bytes) {
        return Result<std::uint64_t>::failure(bytes.error());
    }

    std::uint64_t value = 0U;
    for (std::size_t index = 0U; index < 8U; ++index) {
        const auto byte = std::to_integer<std::uint8_t>(bytes.value()[index]);
        value |= static_cast<std::uint64_t>(byte) << (index * 8U);
    }
    return Result<std::uint64_t>::success(value);
}

Result<float> BinaryReader::read_f32() {
    const Result<std::uint32_t> bits = read_u32();
    if (!bits) {
        return Result<float>::failure(bits.error());
    }
    return Result<float>::success(std::bit_cast<float>(bits.value()));
}

Result<double> BinaryReader::read_f64() {
    const Result<std::uint64_t> bits = read_u64();
    if (!bits) {
        return Result<double>::failure(bits.error());
    }
    return Result<double>::success(std::bit_cast<double>(bits.value()));
}

Result<std::span<const std::byte>> BinaryReader::read_bytes(std::size_t count) {
    if (offset_ > bytes_.size() || count > bytes_.size() - offset_) {
        return Result<std::span<const std::byte>>::failure(
            malformed_archive_error("The archive ended before a complete field was read"));
    }

    const std::span<const std::byte> result = bytes_.subspan(offset_, count);
    offset_ += count;
    return Result<std::span<const std::byte>>::success(result);
}

Result<std::string> BinaryReader::read_string(std::size_t maximum_string_size) {
    const std::size_t original_offset = offset_;
    const Result<std::uint32_t> length_result = read_u32();
    if (!length_result) {
        return Result<std::string>::failure(length_result.error());
    }

    const std::size_t length = static_cast<std::size_t>(length_result.value());
    if (length > maximum_string_size) {
        offset_ = original_offset;
        return Result<std::string>::failure(archive_limit_error(
            "An encoded string exceeds its configured size limit"));
    }

    const Result<std::span<const std::byte>> bytes = read_bytes(length);
    if (!bytes) {
        offset_ = original_offset;
        return Result<std::string>::failure(bytes.error());
    }

    try {
        std::string value;
        if (!bytes.value().empty()) {
            value.assign(
                reinterpret_cast<const char*>(bytes.value().data()),
                bytes.value().size());
        }
        return Result<std::string>::success(std::move(value));
    } catch (const std::bad_alloc&) {
        offset_ = original_offset;
        return Result<std::string>::failure(archive_limit_error(
            "The archive reader could not allocate string storage"));
    } catch (const std::length_error&) {
        offset_ = original_offset;
        return Result<std::string>::failure(archive_limit_error(
            "The decoded string exceeds the container size limit"));
    }
}

std::size_t BinaryReader::remaining() const noexcept {
    return offset_ <= bytes_.size() ? bytes_.size() - offset_ : 0U;
}

bool BinaryReader::empty() const noexcept {
    return remaining() == 0U;
}

} // namespace Storm::Serialization
