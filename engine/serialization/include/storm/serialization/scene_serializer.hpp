#pragma once

#include "storm/core/scene.hpp"
#include "storm/math/transform1d.hpp"
#include "storm/math/transform2d.hpp"
#include "storm/math/transform3d.hpp"
#include "storm/serialization/binary_archive.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <limits>
#include <iterator>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Storm::Serialization {

struct SceneArchiveLimits final {
    std::size_t maximum_archive_bytes{64U * 1024U * 1024U};
    std::uint32_t maximum_entities{250'000U};
    std::uint32_t maximum_component_types{256U};
    std::uint32_t maximum_component_records{1'000'000U};
    std::uint32_t maximum_string_bytes{1U * 1024U * 1024U};
    std::uint32_t maximum_component_bytes{4U * 1024U * 1024U};
};

// Specialize this trait for every component that may be archived.
// type_id must be a stable project-owned identifier, not a C++ type name.
template <typename Component>
struct ComponentCodec;

template <>
struct ComponentCodec<Math::Transform3D> final {
    static constexpr std::string_view type_id{"storm.math.transform3d"};
    static constexpr std::uint32_t schema_version{1U};

    [[nodiscard]] static Result<void> encode(
        const Math::Transform3D& value,
        BinaryWriter& writer) {
        const Result<Math::Matrix4> validation = value.to_matrix();
        if (!validation) {
            return Result<void>::failure(validation.error());
        }

        const std::array<float, 10U> fields{
            value.translation.x,
            value.translation.y,
            value.translation.z,
            value.rotation.x,
            value.rotation.y,
            value.rotation.z,
            value.rotation.w,
            value.scale.x,
            value.scale.y,
            value.scale.z,
        };

        for (const float field : fields) {
            const Result<void> written = writer.write_f32(field);
            if (!written) {
                return written;
            }
        }
        return Result<void>::success();
    }

    [[nodiscard]] static Result<Math::Transform3D> decode(
        std::uint32_t version,
        BinaryReader& reader) {
        if (version != schema_version) {
            return Result<Math::Transform3D>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Transform3D component schema version is unsupported"));
        }

        std::array<float, 10U> fields{};
        for (float& field : fields) {
            const Result<float> read = reader.read_f32();
            if (!read) {
                return Result<Math::Transform3D>::failure(read.error());
            }
            field = read.value();
        }

        Math::Transform3D value{
            .translation = {fields[0U], fields[1U], fields[2U]},
            .rotation = {fields[3U], fields[4U], fields[5U], fields[6U]},
            .scale = {fields[7U], fields[8U], fields[9U]},
        };

        const Result<Math::Matrix4> validation = value.to_matrix();
        if (!validation) {
            return Result<Math::Transform3D>::failure(validation.error());
        }
        return Result<Math::Transform3D>::success(value);
    }
};

template <>
struct ComponentCodec<Math::Transform1D> final {
    static constexpr std::string_view type_id{"storm.math.transform1d"};
    static constexpr std::uint32_t schema_version{1U};

    [[nodiscard]] static Result<void> encode(
        const Math::Transform1D& value,
        BinaryWriter& writer) {
        if (!value.is_valid()) {
            return Result<void>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "The Transform1D position must be finite"));
        }
        return writer.write_f64(value.position);
    }

    [[nodiscard]] static Result<Math::Transform1D> decode(
        std::uint32_t version,
        BinaryReader& reader) {
        if (version != schema_version) {
            return Result<Math::Transform1D>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Transform1D component schema version is unsupported"));
        }

        const Result<double> position = reader.read_f64();
        if (!position) {
            return Result<Math::Transform1D>::failure(position.error());
        }

        Math::Transform1D value{.position = position.value()};
        if (!value.is_valid()) {
            return Result<Math::Transform1D>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "The archived Transform1D position must be finite"));
        }
        return Result<Math::Transform1D>::success(value);
    }
};


template <>
struct ComponentCodec<Math::Transform2D> final {
    static constexpr std::string_view type_id{"storm.math.transform2d"};
    static constexpr std::uint32_t schema_version{1U};
    [[nodiscard]] static Result<void> encode(
        const Math::Transform2D& value, BinaryWriter& writer) {
        const auto valid = value.to_matrix();
        if (!valid) return Result<void>::failure(valid.error());
        const std::array<float, 5U> fields{
            value.translation.x, value.translation.y, value.rotation_radians,
            value.scale.x, value.scale.y};
        for (float field : fields) {
            const auto result = writer.write_f32(field);
            if (!result) return result;
        }
        return Result<void>::success();
    }
    [[nodiscard]] static Result<Math::Transform2D> decode(
        std::uint32_t version, BinaryReader& reader) {
        if (version != schema_version) return Result<Math::Transform2D>::failure(
            Error::make(ErrorCode::Unsupported,
                "The Transform2D component schema version is unsupported"));
        std::array<float, 5U> fields{};
        for (float& field : fields) {
            const auto result = reader.read_f32();
            if (!result) return Result<Math::Transform2D>::failure(result.error());
            field = result.value();
        }
        Math::Transform2D value{
            .translation = {fields[0U], fields[1U]},
            .rotation_radians = fields[2U],
            .scale = {fields[3U], fields[4U]}};
        const auto valid = value.to_matrix();
        if (!valid) return Result<Math::Transform2D>::failure(valid.error());
        return Result<Math::Transform2D>::success(value);
    }
};

namespace detail {

inline constexpr std::uint32_t kNoEntity = std::numeric_limits<std::uint32_t>::max();
inline constexpr std::uint16_t kSceneArchiveVersion = 1U;
inline constexpr std::array<std::byte, 4U> kSceneArchiveMagic{
    std::byte{'S'}, std::byte{'T'}, std::byte{'S'}, std::byte{'C'},
};

template <typename Component>
concept HasComponentCodec = requires(
    const Component& value,
    BinaryWriter& writer,
    BinaryReader& reader) {
    { ComponentCodec<Component>::type_id } -> std::convertible_to<std::string_view>;
    { ComponentCodec<Component>::schema_version } -> std::convertible_to<std::uint32_t>;
    { ComponentCodec<Component>::encode(value, writer) } -> std::same_as<Result<void>>;
    { ComponentCodec<Component>::decode(
        std::uint32_t{1U}, reader) } -> std::same_as<Result<Component>>;
};

template <typename... Components>
consteval bool unique_codec_ids() {
    const std::array<std::string_view, sizeof...(Components)> ids{
        ComponentCodec<Components>::type_id...,
    };
    for (std::size_t left = 0U; left < ids.size(); ++left) {
        if (ids[left].empty()) {
            return false;
        }
        for (std::size_t right = left + 1U; right < ids.size(); ++right) {
            if (ids[left] == ids[right]) {
                return false;
            }
        }
    }
    return true;
}

struct EntitySnapshot final {
    EntityHandle handle;
    std::string name;
    std::uint32_t parent_id{kNoEntity};
    std::uint32_t sibling_order{0U};
};

struct EncodedComponent final {
    std::uint32_t entity_id{0U};
    std::vector<std::byte> payload;
};

struct EncodedComponentBlock final {
    std::string type_id;
    std::uint32_t schema_version{0U};
    std::vector<EncodedComponent> records;
};

[[nodiscard]] inline Error invalid_archive(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

[[nodiscard]] inline Error archive_limit(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

template <typename Entities>
[[nodiscard]] std::optional<std::uint32_t> find_entity_id(
    const Entities& entities,
    EntityHandle handle) {
    const auto iterator = std::lower_bound(
        entities.begin(),
        entities.end(),
        handle.index,
        [](const EntitySnapshot& entity, EntityIndex index) {
            return entity.handle.index < index;
        });

    if (iterator == entities.end() || iterator->handle != handle) {
        return std::nullopt;
    }

    const std::size_t offset = static_cast<std::size_t>(
        std::distance(entities.begin(), iterator));
    if (offset >= static_cast<std::size_t>(kNoEntity)) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(offset);
}

template <typename Component>
[[nodiscard]] Result<std::optional<EncodedComponentBlock>> encode_component_block(
    const Scene& scene,
    const std::vector<EntitySnapshot>& entities,
    const SceneArchiveLimits& limits,
    std::size_t& total_component_records) {
    if (!scene.has_component_storage<Component>()) {
        return Result<std::optional<EncodedComponentBlock>>::success(std::nullopt);
    }

    const std::string_view type_id = ComponentCodec<Component>::type_id;
    if (type_id.empty() || type_id.size() > limits.maximum_string_bytes) {
        return Result<std::optional<EncodedComponentBlock>>::failure(
            archive_limit("A component codec identifier exceeds the configured string limit"));
    }

    EncodedComponentBlock block{
        .type_id = std::string(type_id),
        .schema_version = ComponentCodec<Component>::schema_version,
        .records = {},
    };

    std::optional<Error> callback_error;
    const Result<std::size_t> visited = scene.for_each_component<Component>(
        [&](EntityHandle handle, const Component& component) {
            if (callback_error.has_value()) {
                return;
            }

            const std::optional<std::uint32_t> entity_id =
                find_entity_id(entities, handle);
            if (!entity_id.has_value()) {
                callback_error = invalid_archive(
                    "A component references an entity absent from the Scene snapshot");
                return;
            }

            if (total_component_records >= limits.maximum_component_records) {
                callback_error = archive_limit(
                    "The Scene exceeds the configured component-record limit");
                return;
            }

            BinaryWriter payload_writer(limits.maximum_component_bytes);
            const Result<void> encoded =
                ComponentCodec<Component>::encode(component, payload_writer);
            if (!encoded) {
                callback_error = encoded.error();
                return;
            }

            block.records.push_back(EncodedComponent{
                .entity_id = *entity_id,
                .payload = std::move(payload_writer).take(),
            });
            ++total_component_records;
        });

    if (!visited) {
        return Result<std::optional<EncodedComponentBlock>>::failure(visited.error());
    }
    if (callback_error.has_value()) {
        return Result<std::optional<EncodedComponentBlock>>::failure(
            std::move(*callback_error));
    }
    if (visited.value() != block.records.size() || block.records.empty()) {
        return Result<std::optional<EncodedComponentBlock>>::failure(
            invalid_archive("The component registry changed during Scene serialization"));
    }

    return Result<std::optional<EncodedComponentBlock>>::success(std::move(block));
}

template <typename... Components>
[[nodiscard]] Result<void> encode_all_component_blocks(
    const Scene& scene,
    const std::vector<EntitySnapshot>& entities,
    const SceneArchiveLimits& limits,
    std::vector<EncodedComponentBlock>& blocks) {
    std::size_t registered_storage_count = 0U;
    ((registered_storage_count +=
        scene.has_component_storage<Components>() ? 1U : 0U), ...);

    if (registered_storage_count != scene.component_type_count()) {
        return Result<void>::failure(Error::make(
            ErrorCode::Unsupported,
            "The Scene contains component types without an explicit archive codec"));
    }

    std::size_t total_records = 0U;
    std::optional<Error> block_error;

    auto append = [&]<typename Component>() {
        if (block_error.has_value()) {
            return;
        }
        const Result<std::optional<EncodedComponentBlock>> block =
            encode_component_block<Component>(scene, entities, limits, total_records);
        if (!block) {
            block_error = block.error();
            return;
        }
        if (block.value().has_value()) {
            blocks.push_back(std::move(*block.value()));
        }
    };

    (append.template operator()<Components>(), ...);

    if (block_error.has_value()) {
        return Result<void>::failure(std::move(*block_error));
    }

    std::sort(
        blocks.begin(),
        blocks.end(),
        [](const EncodedComponentBlock& left, const EncodedComponentBlock& right) {
            return left.type_id < right.type_id;
        });

    if (blocks.size() > limits.maximum_component_types ||
        blocks.size() > static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max())) {
        return Result<void>::failure(archive_limit(
            "The Scene exceeds the configured component-type limit"));
    }

    return Result<void>::success();
}

template <typename Component>
[[nodiscard]] Result<void> decode_component_block(
    std::uint32_t schema_version,
    BinaryReader& archive_reader,
    Scene& scene,
    const std::vector<EntityHandle>& entities,
    const SceneArchiveLimits& limits,
    std::size_t& total_component_records) {
    const Result<std::uint32_t> count_result = archive_reader.read_u32();
    if (!count_result) {
        return Result<void>::failure(count_result.error());
    }

    const std::uint32_t record_count = count_result.value();
    if (record_count == 0U ||
        static_cast<std::size_t>(record_count) > entities.size()) {
        return Result<void>::failure(invalid_archive(
            "A component block has an invalid record count"));
    }

    if (total_component_records > limits.maximum_component_records ||
        static_cast<std::size_t>(record_count) >
            static_cast<std::size_t>(limits.maximum_component_records) -
                total_component_records) {
        return Result<void>::failure(archive_limit(
            "The archive exceeds the configured total component-record limit"));
    }

    for (std::uint32_t index = 0U; index < record_count; ++index) {
        const Result<std::uint32_t> entity_id_result = archive_reader.read_u32();
        if (!entity_id_result) {
            return Result<void>::failure(entity_id_result.error());
        }

        const Result<std::uint32_t> payload_size_result = archive_reader.read_u32();
        if (!payload_size_result) {
            return Result<void>::failure(payload_size_result.error());
        }

        if (payload_size_result.value() > limits.maximum_component_bytes) {
            return Result<void>::failure(archive_limit(
                "A component payload exceeds the configured byte limit"));
        }

        const std::size_t entity_id =
            static_cast<std::size_t>(entity_id_result.value());
        if (entity_id >= entities.size()) {
            return Result<void>::failure(invalid_archive(
                "A component record references an unknown entity identifier"));
        }

        const Result<std::span<const std::byte>> payload =
            archive_reader.read_bytes(static_cast<std::size_t>(
                payload_size_result.value()));
        if (!payload) {
            return Result<void>::failure(payload.error());
        }

        BinaryReader payload_reader(payload.value());
        Result<Component> decoded =
            ComponentCodec<Component>::decode(schema_version, payload_reader);
        if (!decoded) {
            return Result<void>::failure(decoded.error());
        }
        if (!payload_reader.empty()) {
            return Result<void>::failure(invalid_archive(
                "A component codec did not consume its complete payload"));
        }

        const Result<std::reference_wrapper<Component>> added =
            scene.add_component<Component>(
                entities[entity_id],
                std::move(decoded).value());
        if (!added) {
            return Result<void>::failure(added.error());
        }

        ++total_component_records;
    }

    return Result<void>::success();
}

template <typename First, typename... Rest>
[[nodiscard]] Result<void> decode_component_type(
    std::string_view type_id,
    std::uint32_t schema_version,
    BinaryReader& archive_reader,
    Scene& scene,
    const std::vector<EntityHandle>& entities,
    const SceneArchiveLimits& limits,
    std::size_t& total_component_records) {
    if (type_id == ComponentCodec<First>::type_id) {
        return decode_component_block<First>(
            schema_version,
            archive_reader,
            scene,
            entities,
            limits,
            total_component_records);
    }

    if constexpr (sizeof...(Rest) > 0U) {
        return decode_component_type<Rest...>(
            type_id,
            schema_version,
            archive_reader,
            scene,
            entities,
            limits,
            total_component_records);
    } else {
        return Result<void>::failure(Error::make(
            ErrorCode::Unsupported,
            "The archive contains a component type without a supplied codec"));
    }
}

} // namespace detail

class SceneSerializer final {
public:
    SceneSerializer() = delete;

    template <typename... Components>
    [[nodiscard]] static Result<std::vector<std::byte>> serialize(
        const Scene& scene,
        const SceneArchiveLimits& limits = {}) {
        static_assert(
            (detail::HasComponentCodec<Components> && ...),
            "Every serialized component requires a ComponentCodec specialization");
        static_assert(
            detail::unique_codec_ids<Components...>(),
            "Component codec identifiers must be non-empty and unique");

        try {
            return serialize_impl<Components...>(scene, limits);
        } catch (const std::bad_alloc&) {
            return Result<std::vector<std::byte>>::failure(detail::archive_limit(
                "The Scene serializer could not allocate its bounded working storage"));
        } catch (const std::length_error&) {
            return Result<std::vector<std::byte>>::failure(detail::archive_limit(
                "The Scene serializer reached a container size limit"));
        }
    }

    template <typename... Components>
    [[nodiscard]] static Result<Scene> deserialize(
        std::span<const std::byte> archive,
        const SceneArchiveLimits& limits = {}) {
        static_assert(
            (detail::HasComponentCodec<Components> && ...),
            "Every deserialized component requires a ComponentCodec specialization");
        static_assert(
            detail::unique_codec_ids<Components...>(),
            "Component codec identifiers must be non-empty and unique");

        try {
            return deserialize_impl<Components...>(archive, limits);
        } catch (const std::bad_alloc&) {
            return Result<Scene>::failure(detail::archive_limit(
                "The Scene deserializer could not allocate its bounded working storage"));
        } catch (const std::length_error&) {
            return Result<Scene>::failure(detail::archive_limit(
                "The Scene deserializer reached a container size limit"));
        }
    }

private:
    template <typename... Components>
    [[nodiscard]] static Result<std::vector<std::byte>> serialize_impl(
        const Scene& scene,
        const SceneArchiveLimits& limits) {
        if (scene.id() == 0U) {
            return Result<std::vector<std::byte>>::failure(Error::make(
                ErrorCode::InvalidState,
                "A moved-from Scene cannot be serialized"));
        }
        if (limits.maximum_archive_bytes < detail::kSceneArchiveMagic.size()) {
            return Result<std::vector<std::byte>>::failure(detail::archive_limit(
                "The configured archive byte limit is too small for a Scene header"));
        }

        std::vector<detail::EntitySnapshot> entities;
        std::optional<Error> entity_error;
        const Result<std::size_t> visited = scene.for_each_entity(
            [&](EntityHandle handle, std::string_view name) {
                if (entity_error.has_value()) {
                    return;
                }
                if (entities.size() >= limits.maximum_entities ||
                    entities.size() >= static_cast<std::size_t>(detail::kNoEntity)) {
                    entity_error = detail::archive_limit(
                        "The Scene exceeds the configured entity-count limit");
                    return;
                }

                const Result<std::optional<EntityHandle>> parent =
                    scene.parent_of(handle);
                if (!parent) {
                    entity_error = parent.error();
                    return;
                }

                entities.push_back(detail::EntitySnapshot{
                    .handle = handle,
                    .name = std::string(name),
                    .parent_id = detail::kNoEntity,
                    .sibling_order = 0U,
                });

                if (parent.value().has_value()) {
                    const std::optional<std::uint32_t> parent_id =
                        detail::find_entity_id(entities, *parent.value());
                    if (parent_id.has_value()) {
                        entities.back().parent_id = *parent_id;
                    } else {
                        // Parent slots may sort after their children. Resolve these
                        // links in a second pass after the complete snapshot exists.
                        entities.back().parent_id = detail::kNoEntity;
                    }
                }
            });

        if (!visited) {
            return Result<std::vector<std::byte>>::failure(visited.error());
        }
        if (entity_error.has_value()) {
            return Result<std::vector<std::byte>>::failure(std::move(*entity_error));
        }
        if (visited.value() != entities.size()) {
            return Result<std::vector<std::byte>>::failure(detail::invalid_archive(
                "The Scene entity set changed during serialization"));
        }

        for (std::size_t child_id = 0U; child_id < entities.size(); ++child_id) {
            const Result<std::optional<EntityHandle>> parent =
                scene.parent_of(entities[child_id].handle);
            if (!parent) {
                return Result<std::vector<std::byte>>::failure(parent.error());
            }
            if (!parent.value().has_value()) {
                entities[child_id].parent_id = detail::kNoEntity;
                continue;
            }

            const std::optional<std::uint32_t> parent_id =
                detail::find_entity_id(entities, *parent.value());
            if (!parent_id.has_value()) {
                return Result<std::vector<std::byte>>::failure(
                    detail::invalid_archive("An entity parent is absent from the Scene"));
            }
            entities[child_id].parent_id = *parent_id;
        }

        std::vector<bool> child_link_seen(entities.size(), false);
        for (std::size_t parent_id = 0U; parent_id < entities.size(); ++parent_id) {
            const Result<std::span<const EntityHandle>> children =
                scene.children_of(entities[parent_id].handle);
            if (!children) {
                return Result<std::vector<std::byte>>::failure(children.error());
            }

            if (children.value().size() >= static_cast<std::size_t>(detail::kNoEntity)) {
                return Result<std::vector<std::byte>>::failure(detail::archive_limit(
                    "A Scene parent has too many direct children to archive"));
            }

            for (std::size_t order = 0U; order < children.value().size(); ++order) {
                const std::optional<std::uint32_t> child_id =
                    detail::find_entity_id(entities, children.value()[order]);
                if (!child_id.has_value() ||
                    entities[*child_id].parent_id != static_cast<std::uint32_t>(parent_id) ||
                    child_link_seen[*child_id]) {
                    return Result<std::vector<std::byte>>::failure(
                        detail::invalid_archive("The Scene hierarchy links are inconsistent"));
                }

                entities[*child_id].sibling_order = static_cast<std::uint32_t>(order);
                child_link_seen[*child_id] = true;
            }
        }

        for (std::size_t index = 0U; index < entities.size(); ++index) {
            const bool has_parent = entities[index].parent_id != detail::kNoEntity;
            if (has_parent != child_link_seen[index]) {
                return Result<std::vector<std::byte>>::failure(
                    detail::invalid_archive("The Scene hierarchy is not bidirectionally consistent"));
            }
        }

        std::vector<detail::EncodedComponentBlock> blocks;
        const Result<void> encoded_blocks =
            detail::encode_all_component_blocks<Components...>(
                scene, entities, limits, blocks);
        if (!encoded_blocks) {
            return Result<std::vector<std::byte>>::failure(encoded_blocks.error());
        }

        BinaryWriter writer(limits.maximum_archive_bytes);
        Result<void> operation = writer.write_bytes(detail::kSceneArchiveMagic);
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }
        operation = writer.write_u16(detail::kSceneArchiveVersion);
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }
        operation = writer.write_u16(0U);
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }
        operation = writer.write_string(scene.name(), limits.maximum_string_bytes);
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }
        operation = writer.write_u32(static_cast<std::uint32_t>(entities.size()));
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }

        for (std::size_t index = 0U; index < entities.size(); ++index) {
            const detail::EntitySnapshot& entity = entities[index];
            operation = writer.write_u32(static_cast<std::uint32_t>(index));
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
            operation = writer.write_string(entity.name, limits.maximum_string_bytes);
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
            operation = writer.write_u32(entity.parent_id);
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
            operation = writer.write_u32(entity.sibling_order);
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
        }

        operation = writer.write_u32(static_cast<std::uint32_t>(blocks.size()));
        if (!operation) {
            return Result<std::vector<std::byte>>::failure(operation.error());
        }

        for (const detail::EncodedComponentBlock& block : blocks) {
            operation = writer.write_string(block.type_id, limits.maximum_string_bytes);
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
            operation = writer.write_u32(block.schema_version);
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }
            operation = writer.write_u32(static_cast<std::uint32_t>(block.records.size()));
            if (!operation) {
                return Result<std::vector<std::byte>>::failure(operation.error());
            }

            for (const detail::EncodedComponent& record : block.records) {
                if (record.payload.size() > limits.maximum_component_bytes ||
                    record.payload.size() > static_cast<std::size_t>(
                        std::numeric_limits<std::uint32_t>::max())) {
                    return Result<std::vector<std::byte>>::failure(detail::archive_limit(
                        "A component payload exceeds the configured byte limit"));
                }
                operation = writer.write_u32(record.entity_id);
                if (!operation) {
                    return Result<std::vector<std::byte>>::failure(operation.error());
                }
                operation = writer.write_u32(static_cast<std::uint32_t>(record.payload.size()));
                if (!operation) {
                    return Result<std::vector<std::byte>>::failure(operation.error());
                }
                operation = writer.write_bytes(record.payload);
                if (!operation) {
                    return Result<std::vector<std::byte>>::failure(operation.error());
                }
            }
        }

        return Result<std::vector<std::byte>>::success(std::move(writer).take());
    }

    template <typename... Components>
    [[nodiscard]] static Result<Scene> deserialize_impl(
        std::span<const std::byte> archive,
        const SceneArchiveLimits& limits) {
        if (archive.size() > limits.maximum_archive_bytes) {
            return Result<Scene>::failure(detail::archive_limit(
                "The input archive exceeds the configured byte limit"));
        }

        BinaryReader reader(archive);
        const Result<std::span<const std::byte>> magic = reader.read_bytes(
            detail::kSceneArchiveMagic.size());
        if (!magic) {
            return Result<Scene>::failure(magic.error());
        }
        if (!std::equal(
                magic.value().begin(),
                magic.value().end(),
                detail::kSceneArchiveMagic.begin())) {
            return Result<Scene>::failure(detail::invalid_archive(
                "The input does not contain a Storm Scene archive header"));
        }

        const Result<std::uint16_t> version = reader.read_u16();
        if (!version) {
            return Result<Scene>::failure(version.error());
        }
        if (version.value() != detail::kSceneArchiveVersion) {
            return Result<Scene>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Storm Scene archive format version is unsupported"));
        }

        const Result<std::uint16_t> flags = reader.read_u16();
        if (!flags) {
            return Result<Scene>::failure(flags.error());
        }
        if (flags.value() != 0U) {
            return Result<Scene>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Storm Scene archive uses unsupported feature flags"));
        }

        const Result<std::string> scene_name =
            reader.read_string(limits.maximum_string_bytes);
        if (!scene_name) {
            return Result<Scene>::failure(scene_name.error());
        }

        const Result<std::uint32_t> entity_count_result = reader.read_u32();
        if (!entity_count_result) {
            return Result<Scene>::failure(entity_count_result.error());
        }
        const std::uint32_t entity_count = entity_count_result.value();
        if (entity_count > limits.maximum_entities ||
            entity_count == detail::kNoEntity) {
            return Result<Scene>::failure(detail::archive_limit(
                "The Scene archive exceeds the configured entity-count limit"));
        }

        struct DecodedEntity final {
            std::string name;
            std::uint32_t parent_id{detail::kNoEntity};
            std::uint32_t sibling_order{0U};
        };

        std::vector<DecodedEntity> decoded_entities;
        decoded_entities.reserve(static_cast<std::size_t>(entity_count));
        std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>>
            children_by_parent(static_cast<std::size_t>(entity_count));

        for (std::uint32_t index = 0U; index < entity_count; ++index) {
            const Result<std::uint32_t> stored_id = reader.read_u32();
            if (!stored_id) {
                return Result<Scene>::failure(stored_id.error());
            }
            if (stored_id.value() != index) {
                return Result<Scene>::failure(detail::invalid_archive(
                    "Scene entity identifiers must be contiguous and ordered"));
            }

            const Result<std::string> entity_name =
                reader.read_string(limits.maximum_string_bytes);
            if (!entity_name) {
                return Result<Scene>::failure(entity_name.error());
            }

            const Result<std::uint32_t> parent_id = reader.read_u32();
            if (!parent_id) {
                return Result<Scene>::failure(parent_id.error());
            }
            const Result<std::uint32_t> sibling_order = reader.read_u32();
            if (!sibling_order) {
                return Result<Scene>::failure(sibling_order.error());
            }

            if (parent_id.value() == detail::kNoEntity) {
                if (sibling_order.value() != 0U) {
                    return Result<Scene>::failure(detail::invalid_archive(
                        "A root entity must have a zero sibling order"));
                }
            } else {
                if (parent_id.value() >= entity_count ||
                    parent_id.value() == index) {
                    return Result<Scene>::failure(detail::invalid_archive(
                        "A Scene entity contains an invalid parent identifier"));
                }
                children_by_parent[static_cast<std::size_t>(parent_id.value())]
                    .emplace_back(sibling_order.value(), index);
            }

            decoded_entities.push_back(DecodedEntity{
                .name = std::move(entity_name).value(),
                .parent_id = parent_id.value(),
                .sibling_order = sibling_order.value(),
            });
        }

        for (auto& children : children_by_parent) {
            std::sort(children.begin(), children.end());
            for (std::size_t index = 0U; index < children.size(); ++index) {
                if (children[index].first != static_cast<std::uint32_t>(index)) {
                    return Result<Scene>::failure(detail::invalid_archive(
                        "Sibling order values must be unique and contiguous for each parent"));
                }
            }
        }

        Result<Scene> scene_result = Scene::create(scene_name.value());
        if (!scene_result) {
            return Result<Scene>::failure(scene_result.error());
        }
        Scene scene = std::move(scene_result).value();

        std::vector<EntityHandle> handles;
        handles.reserve(static_cast<std::size_t>(entity_count));
        for (const DecodedEntity& entity : decoded_entities) {
            const Result<EntityHandle> created =
                scene.create_entity(entity.name);
            if (!created) {
                return Result<Scene>::failure(created.error());
            }
            handles.push_back(created.value());
        }

        for (std::size_t parent_id = 0U;
             parent_id < children_by_parent.size();
             ++parent_id) {
            for (const auto& [sibling_order, child_id] : children_by_parent[parent_id]) {
                static_cast<void>(sibling_order);
                const Result<void> attached = scene.set_parent(
                    handles[static_cast<std::size_t>(child_id)],
                    handles[parent_id]);
                if (!attached) {
                    return Result<Scene>::failure(attached.error());
                }
            }
        }

        const Result<std::uint32_t> type_count_result = reader.read_u32();
        if (!type_count_result) {
            return Result<Scene>::failure(type_count_result.error());
        }
        if (type_count_result.value() > limits.maximum_component_types ||
            type_count_result.value() > sizeof...(Components)) {
            return Result<Scene>::failure(Error::make(
                ErrorCode::Unsupported,
                "The archive contains more component types than supplied codecs"));
        }

        std::string previous_type_id;
        std::size_t total_component_records = 0U;
        for (std::uint32_t type_index = 0U;
             type_index < type_count_result.value();
             ++type_index) {
            const Result<std::string> type_id =
                reader.read_string(limits.maximum_string_bytes);
            if (!type_id) {
                return Result<Scene>::failure(type_id.error());
            }
            if (type_id.value().empty() ||
                (!previous_type_id.empty() && type_id.value() <= previous_type_id)) {
                return Result<Scene>::failure(detail::invalid_archive(
                    "Component type identifiers must be non-empty, unique, and sorted"));
            }

            const Result<std::uint32_t> schema_version = reader.read_u32();
            if (!schema_version) {
                return Result<Scene>::failure(schema_version.error());
            }

            const Result<void> decoded = [&]() -> Result<void> {
                if constexpr (sizeof...(Components) == 0U) {
                    return Result<void>::failure(Error::make(
                        ErrorCode::Unsupported,
                        "The archive contains components but no component codecs were supplied"));
                } else {
                    return detail::decode_component_type<Components...>(
                        type_id.value(),
                        schema_version.value(),
                        reader,
                        scene,
                        handles,
                        limits,
                        total_component_records);
                }
            }();
            if (!decoded) {
                return Result<Scene>::failure(decoded.error());
            }

            previous_type_id = type_id.value();
        }

        if (!reader.empty()) {
            return Result<Scene>::failure(detail::invalid_archive(
                "The Scene archive contains trailing bytes"));
        }

        return Result<Scene>::success(std::move(scene));
    }
};

} // namespace Storm::Serialization
