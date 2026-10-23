#pragma once

#include "storm/scene/scene.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace storm::scene {

static_assert(sizeof(float) == sizeof(std::uint32_t) &&
              std::numeric_limits<float>::is_iec559,
              "SceneCodec requires IEEE-754 binary32 floats");

// Versioned, deterministic scene persistence. Version 1 is a bounded little-endian binary format.
// Deserialization is transactional and accepts only an empty destination Scene with matching
// dimension and enough entity capacity.
class SceneCodec final {
public:
    static constexpr std::uint16_t format_version = 1U;
    static constexpr std::size_t maximum_serialized_bytes = 64U * 1024U * 1024U;

    [[nodiscard]] static core::Status serialize(const Scene& scene,
                                                std::vector<std::byte>& output) {
        const core::Status valid_scene = scene.validate();
        if (!valid_scene.ok()) {
            return valid_scene;
        }
        if (scene.entity_count() >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "scene has too many entities to serialize");
        }

        const std::size_t transform_bytes = float_count(scene.dimension()) * sizeof(std::uint32_t);
        std::size_t encoded_size = header_size;
        for (const auto& entry : scene.entities_) {
            const Entity& entity = entry.second;
            if (!Entity::is_valid_name(entity.name_) || !entity.transform_.is_finite() ||
                entity.transform_.dimension() != scene.dimension()) {
                return core::Status::failure(
                    core::ErrorCode::InvalidState, "scene contains an invalid entity record");
            }

            const std::size_t record_size = core::StableId::byte_count + sizeof(std::uint16_t) +
                                            entity.name_.size() + transform_bytes;
            if (record_size > maximum_serialized_bytes - encoded_size) {
                return core::Status::failure(
                    core::ErrorCode::ResourceExhausted, "serialized scene exceeds the size limit");
            }
            encoded_size += record_size;
        }
        if (encoded_size > maximum_serialized_bytes) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "serialized scene exceeds the size limit");
        }

        try {
            std::vector<std::byte> encoded;
            encoded.reserve(encoded_size);
            append_u8(encoded, 0x53U); // S
            append_u8(encoded, 0x54U); // T
            append_u8(encoded, 0x53U); // S
            append_u8(encoded, 0x43U); // C
            append_u16(encoded, format_version);
            append_u8(encoded, static_cast<std::uint8_t>(scene.dimension()));
            append_u8(encoded, 0U); // Reserved flags must remain zero in version 1.
            append_u32(encoded, static_cast<std::uint32_t>(scene.entity_count()));

            // Scene storage is ordered by StableIdLess, making serialization reproducible.
            for (const auto& entry : scene.entities_) {
                const Entity& entity = entry.second;
                for (const std::uint8_t byte : entity.id_.bytes()) {
                    append_u8(encoded, byte);
                }
                append_u16(encoded, static_cast<std::uint16_t>(entity.name_.size()));
                for (const char character : entity.name_) {
                    append_u8(encoded, static_cast<std::uint8_t>(
                                           static_cast<unsigned char>(character)));
                }

                std::visit(
                    [&encoded](const auto& value) {
                        using Value = std::remove_cvref_t<decltype(value)>;
                        if constexpr (std::is_same_v<Value, Transform1D>) {
                            append_float(encoded, value.position);
                            append_float(encoded, value.scale);
                        } else if constexpr (std::is_same_v<Value, Transform2D>) {
                            append_float(encoded, value.position[0]);
                            append_float(encoded, value.position[1]);
                            append_float(encoded, value.rotation_radians);
                            append_float(encoded, value.scale[0]);
                            append_float(encoded, value.scale[1]);
                        } else {
                            for (const float component : value.position.values()) {
                                append_float(encoded, component);
                            }
                            for (const float component : value.rotation_euler_radians.values()) {
                                append_float(encoded, component);
                            }
                            for (const float component : value.scale.values()) {
                                append_float(encoded, component);
                            }
                        }
                    },
                    entity.transform_.data());
            }

            if (encoded.size() != encoded_size) {
                return core::Status::failure(
                    core::ErrorCode::Internal, "scene serialization size calculation disagrees");
            }
            output.swap(encoded);
        } catch (const std::bad_alloc&) {
            return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
        }

        return core::Status::success();
    }

    // The destination must be empty, match the file's dimension, and have enough capacity.
    // On any malformed input or allocation failure, the destination remains unchanged.
    [[nodiscard]] static core::Status deserialize(std::span<const std::byte> bytes,
                                                  Scene& destination) {
        const core::Status valid_scene = destination.validate();
        if (!valid_scene.ok()) {
            return valid_scene;
        }
        if (!destination.entities_.empty()) {
            return core::Status::failure(
                core::ErrorCode::InvalidState, "destination scene must be empty");
        }
        if (bytes.size() > maximum_serialized_bytes) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "serialized scene exceeds the size limit");
        }
        if (bytes.size() < header_size) {
            return core::Status::failure(core::ErrorCode::Parse, "scene header is truncated");
        }

        Reader reader{bytes};
        std::array<std::uint8_t, 4U> magic{};
        for (std::uint8_t& value : magic) {
            if (!reader.read_u8(value)) {
                return core::Status::failure(core::ErrorCode::Parse, "scene header is truncated");
            }
        }
        if (magic != magic_bytes) {
            return core::Status::failure(core::ErrorCode::Parse, "scene magic is invalid");
        }

        std::uint16_t version{};
        std::uint8_t dimension_byte{};
        std::uint8_t reserved{};
        std::uint32_t entity_count{};
        if (!reader.read_u16(version) || !reader.read_u8(dimension_byte) ||
            !reader.read_u8(reserved) || !reader.read_u32(entity_count)) {
            return core::Status::failure(core::ErrorCode::Parse, "scene header is truncated");
        }
        if (version != format_version) {
            return core::Status::failure(
                core::ErrorCode::Unsupported, "scene format version is not supported");
        }
        if (reserved != 0U) {
            return core::Status::failure(core::ErrorCode::Parse, "scene reserved flags are nonzero");
        }

        const Dimension file_dimension = static_cast<Dimension>(dimension_byte);
        if (!is_valid_dimension(file_dimension)) {
            return core::Status::failure(core::ErrorCode::Parse, "scene dimension is invalid");
        }
        if (file_dimension != destination.dimension()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "scene dimension does not match destination");
        }
        if (static_cast<std::size_t>(entity_count) > destination.max_entities() ||
            static_cast<std::size_t>(entity_count) > Scene::maximum_allowed_entities) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "scene entity count exceeds destination capacity");
        }

        const std::size_t minimum_record_size = core::StableId::byte_count +
                                                sizeof(std::uint16_t) + 1U +
                                                float_count(file_dimension) * sizeof(std::uint32_t);
        if (static_cast<std::size_t>(entity_count) > reader.remaining() / minimum_record_size) {
            return core::Status::failure(
                core::ErrorCode::Parse, "scene entity count exceeds the available file data");
        }

        try {
            std::map<core::StableId, Entity, core::StableIdLess> parsed_entities;
            std::optional<core::StableId> previous_id;
            const core::StableIdLess id_less{};

            for (std::uint32_t index = 0U; index < entity_count; ++index) {
                core::StableId::Bytes id_bytes{};
                for (std::uint8_t& value : id_bytes) {
                    if (!reader.read_u8(value)) {
                        return core::Status::failure(
                            core::ErrorCode::Parse, "entity identifier is truncated");
                    }
                }
                const auto id = core::StableId::from_bytes(id_bytes);
                if (!id.has_value()) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity identifier is invalid");
                }
                if (previous_id.has_value() && !id_less(*previous_id, *id)) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity identifiers are not strictly ordered");
                }

                std::uint16_t name_length{};
                if (!reader.read_u16(name_length) || name_length == 0U ||
                    name_length > Entity::max_name_bytes ||
                    static_cast<std::size_t>(name_length) > reader.remaining()) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity name length is invalid");
                }
                std::string name{
                    reinterpret_cast<const char*>(reader.current_data()),
                    static_cast<std::size_t>(name_length)};
                if (!reader.skip(static_cast<std::size_t>(name_length)) ||
                    !Entity::is_valid_name(name)) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity name contains invalid bytes");
                }

                Transform transform;
                if (file_dimension == Dimension::One) {
                    float position{};
                    float scale{};
                    if (!reader.read_float(position) || !reader.read_float(scale)) {
                        return core::Status::failure(
                            core::ErrorCode::Parse, "1D transform is truncated");
                    }
                    transform = Transform{Transform1D{position, scale}};
                } else if (file_dimension == Dimension::Two) {
                    float x{};
                    float y{};
                    float rotation{};
                    float scale_x{};
                    float scale_y{};
                    if (!reader.read_float(x) || !reader.read_float(y) ||
                        !reader.read_float(rotation) || !reader.read_float(scale_x) ||
                        !reader.read_float(scale_y)) {
                        return core::Status::failure(
                            core::ErrorCode::Parse, "2D transform is truncated");
                    }
                    transform = Transform{Transform2D{
                        math::Vec2{x, y}, rotation, math::Vec2{scale_x, scale_y}}};
                } else {
                    std::array<float, 3U> position{};
                    std::array<float, 3U> rotation{};
                    std::array<float, 3U> scale{};
                    for (float& component : position) {
                        if (!reader.read_float(component)) {
                            return core::Status::failure(
                                core::ErrorCode::Parse, "3D transform is truncated");
                        }
                    }
                    for (float& component : rotation) {
                        if (!reader.read_float(component)) {
                            return core::Status::failure(
                                core::ErrorCode::Parse, "3D transform is truncated");
                        }
                    }
                    for (float& component : scale) {
                        if (!reader.read_float(component)) {
                            return core::Status::failure(
                                core::ErrorCode::Parse, "3D transform is truncated");
                        }
                    }
                    transform = Transform{Transform3D{
                        math::Vec3{position[0], position[1], position[2]},
                        math::Vec3{rotation[0], rotation[1], rotation[2]},
                        math::Vec3{scale[0], scale[1], scale[2]}}};
                }
                if (!transform.is_finite()) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity transform contains a non-finite value");
                }

                const auto inserted = parsed_entities.emplace(
                    *id, Entity{*id, std::move(name), std::move(transform)});
                if (!inserted.second) {
                    return core::Status::failure(
                        core::ErrorCode::Parse, "entity identifier is duplicated");
                }
                previous_id = *id;
            }

            if (!reader.empty()) {
                return core::Status::failure(
                    core::ErrorCode::Parse, "scene contains trailing data");
            }
            destination.entities_.swap(parsed_entities);
        } catch (const std::bad_alloc&) {
            return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
        }

        return core::Status::success();
    }

private:
    static constexpr std::size_t header_size = 12U;
    static constexpr std::array<std::uint8_t, 4U> magic_bytes{0x53U, 0x54U, 0x53U, 0x43U};

    [[nodiscard]] static constexpr std::size_t float_count(Dimension dimension) noexcept {
        switch (dimension) {
        case Dimension::One:
            return 2U;
        case Dimension::Two:
            return 5U;
        case Dimension::Three:
            return 9U;
        }
        return 0U;
    }

    static void append_u8(std::vector<std::byte>& output, std::uint8_t value) {
        output.push_back(static_cast<std::byte>(value));
    }

    static void append_u16(std::vector<std::byte>& output, std::uint16_t value) {
        append_u8(output, static_cast<std::uint8_t>(value & 0xffU));
        append_u8(output, static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    }

    static void append_u32(std::vector<std::byte>& output, std::uint32_t value) {
        append_u8(output, static_cast<std::uint8_t>(value & 0xffU));
        append_u8(output, static_cast<std::uint8_t>((value >> 8U) & 0xffU));
        append_u8(output, static_cast<std::uint8_t>((value >> 16U) & 0xffU));
        append_u8(output, static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    }

    static void append_float(std::vector<std::byte>& output, float value) {
        append_u32(output, std::bit_cast<std::uint32_t>(value));
    }

    class Reader final {
    public:
        explicit Reader(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}

        [[nodiscard]] bool read_u8(std::uint8_t& output) noexcept {
            if (remaining() < 1U) {
                return false;
            }
            output = std::to_integer<std::uint8_t>(bytes_[position_]);
            ++position_;
            return true;
        }

        [[nodiscard]] bool read_u16(std::uint16_t& output) noexcept {
            std::uint8_t low{};
            std::uint8_t high{};
            if (!read_u8(low) || !read_u8(high)) {
                return false;
            }
            output = static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(low) |
                static_cast<std::uint16_t>(static_cast<unsigned int>(high) << 8U));
            return true;
        }

        [[nodiscard]] bool read_u32(std::uint32_t& output) noexcept {
            std::uint8_t b0{};
            std::uint8_t b1{};
            std::uint8_t b2{};
            std::uint8_t b3{};
            if (!read_u8(b0) || !read_u8(b1) || !read_u8(b2) || !read_u8(b3)) {
                return false;
            }
            output = static_cast<std::uint32_t>(b0) |
                     (static_cast<std::uint32_t>(b1) << 8U) |
                     (static_cast<std::uint32_t>(b2) << 16U) |
                     (static_cast<std::uint32_t>(b3) << 24U);
            return true;
        }

        [[nodiscard]] bool read_float(float& output) noexcept {
            std::uint32_t bits{};
            if (!read_u32(bits)) {
                return false;
            }
            output = std::bit_cast<float>(bits);
            return true;
        }

        [[nodiscard]] bool skip(std::size_t size) noexcept {
            if (size > remaining()) {
                return false;
            }
            position_ += size;
            return true;
        }

        [[nodiscard]] std::size_t remaining() const noexcept {
            return bytes_.size() - position_;
        }

        [[nodiscard]] bool empty() const noexcept {
            return position_ == bytes_.size();
        }

        [[nodiscard]] const std::byte* current_data() const noexcept {
            return bytes_.data() + position_;
        }

    private:
        std::span<const std::byte> bytes_;
        std::size_t position_{};
    };
};

} // namespace storm::scene
