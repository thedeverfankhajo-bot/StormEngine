#include "storm/serialization/scene_serializer.hpp"
#include "storm/core/transform_system.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace TestComponents {

struct Position final {
    float x{0.0F};
    float y{0.0F};
};

struct Velocity final {
    float value{0.0F};
};

} // namespace TestComponents

namespace Storm::Serialization {

template <>
struct ComponentCodec<TestComponents::Position> final {
    static constexpr std::string_view type_id{"test.position"};
    static constexpr std::uint32_t schema_version{1U};

    [[nodiscard]] static Result<void> encode(
        const TestComponents::Position& value,
        BinaryWriter& writer) {
        Result<void> result = writer.write_f32(value.x);
        if (!result) {
            return result;
        }
        return writer.write_f32(value.y);
    }

    [[nodiscard]] static Result<TestComponents::Position> decode(
        std::uint32_t version,
        BinaryReader& reader) {
        if (version != schema_version) {
            return Result<TestComponents::Position>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Position component schema version is unsupported"));
        }

        const Result<float> x = reader.read_f32();
        if (!x) {
            return Result<TestComponents::Position>::failure(x.error());
        }
        const Result<float> y = reader.read_f32();
        if (!y) {
            return Result<TestComponents::Position>::failure(y.error());
        }
        if (!std::isfinite(x.value()) || !std::isfinite(y.value())) {
            return Result<TestComponents::Position>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "Position values must be finite"));
        }
        return Result<TestComponents::Position>::success(
            TestComponents::Position{x.value(), y.value()});
    }
};

template <>
struct ComponentCodec<TestComponents::Velocity> final {
    static constexpr std::string_view type_id{"test.velocity"};
    static constexpr std::uint32_t schema_version{1U};

    [[nodiscard]] static Result<void> encode(
        const TestComponents::Velocity& value,
        BinaryWriter& writer) {
        return writer.write_f32(value.value);
    }

    [[nodiscard]] static Result<TestComponents::Velocity> decode(
        std::uint32_t version,
        BinaryReader& reader) {
        if (version != schema_version) {
            return Result<TestComponents::Velocity>::failure(Error::make(
                ErrorCode::Unsupported,
                "The Velocity component schema version is unsupported"));
        }
        const Result<float> value = reader.read_f32();
        if (!value) {
            return Result<TestComponents::Velocity>::failure(value.error());
        }
        if (!std::isfinite(value.value())) {
            return Result<TestComponents::Velocity>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "Velocity must be finite"));
        }
        return Result<TestComponents::Velocity>::success(
            TestComponents::Velocity{value.value()});
    }
};

} // namespace Storm::Serialization

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

using Storm::Serialization::BinaryWriter;
using Storm::Serialization::SceneArchiveLimits;
using Storm::Serialization::SceneSerializer;

struct RawRecord final {
    std::uint32_t entity_id{0U};
    std::vector<std::byte> payload;
};

struct RawBlock final {
    std::string type_id;
    std::uint32_t version{1U};
    std::vector<RawRecord> records;
};

struct RawEntity final {
    std::string name;
    std::uint32_t parent{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t sibling_order{0U};
};

[[nodiscard]] std::optional<std::vector<std::byte>> make_raw_archive(
    std::string_view scene_name,
    const std::vector<RawEntity>& entities,
    const std::vector<RawBlock>& blocks) {
    BinaryWriter writer(8U * 1024U * 1024U);
    auto append = [](Storm::Result<void> result) {
        return static_cast<bool>(result);
    };

    const std::array<std::byte, 4U> magic{
        static_cast<std::byte>('S'),
        static_cast<std::byte>('T'),
        static_cast<std::byte>('S'),
        static_cast<std::byte>('C'),
    };
    if (!append(writer.write_bytes(magic)) ||
        !append(writer.write_u16(1U)) ||
        !append(writer.write_u16(0U)) ||
        !append(writer.write_string(scene_name, 4096U)) ||
        !append(writer.write_u32(static_cast<std::uint32_t>(entities.size())))) {
        return std::nullopt;
    }

    for (std::size_t index = 0U; index < entities.size(); ++index) {
        if (!append(writer.write_u32(static_cast<std::uint32_t>(index))) ||
            !append(writer.write_string(entities[index].name, 4096U)) ||
            !append(writer.write_u32(entities[index].parent)) ||
            !append(writer.write_u32(entities[index].sibling_order))) {
            return std::nullopt;
        }
    }

    if (!append(writer.write_u32(static_cast<std::uint32_t>(blocks.size())))) {
        return std::nullopt;
    }
    for (const RawBlock& block : blocks) {
        if (!append(writer.write_string(block.type_id, 4096U)) ||
            !append(writer.write_u32(block.version)) ||
            !append(writer.write_u32(static_cast<std::uint32_t>(block.records.size())))) {
            return std::nullopt;
        }
        for (const RawRecord& record : block.records) {
            if (!append(writer.write_u32(record.entity_id)) ||
                !append(writer.write_u32(static_cast<std::uint32_t>(record.payload.size()))) ||
                !append(writer.write_bytes(record.payload))) {
                return std::nullopt;
            }
        }
    }

    return std::move(writer).take();
}

[[nodiscard]] std::vector<std::byte> position_payload(float x, float y) {
    BinaryWriter writer(64U);
    static_cast<void>(writer.write_f32(x));
    static_cast<void>(writer.write_f32(y));
    return std::move(writer).take();
}

[[nodiscard]] bool replace_ascii(
    std::vector<std::byte>& bytes,
    std::string_view old_value,
    std::string_view new_value) {
    if (old_value.size() != new_value.size() || old_value.empty()) {
        return false;
    }
    for (std::size_t offset = 0U; offset + old_value.size() <= bytes.size(); ++offset) {
        bool matches = true;
        for (std::size_t index = 0U; index < old_value.size(); ++index) {
            if (std::to_integer<unsigned char>(bytes[offset + index]) !=
                static_cast<unsigned char>(old_value[index])) {
                matches = false;
                break;
            }
        }
        if (matches) {
            for (std::size_t index = 0U; index < new_value.size(); ++index) {
                bytes[offset + index] = static_cast<std::byte>(new_value[index]);
            }
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::optional<std::string> test_round_trip_preserves_hierarchy_components_and_transforms() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    Storm::Scene scene = std::move(scene_result).value();

    auto root = scene.create_entity("Root");
    auto first_child = scene.create_entity("FirstChild");
    auto second_child = scene.create_entity("SecondChild");
    EXPECT(root);
    EXPECT(first_child);
    EXPECT(second_child);

    EXPECT(scene.set_parent(second_child.value(), root.value()));
    EXPECT(scene.set_parent(first_child.value(), root.value()));
    EXPECT(scene.add_component<TestComponents::Position>(
        first_child.value(), TestComponents::Position{12.5F, -4.0F}));
    EXPECT(scene.add_component<TestComponents::Velocity>(
        second_child.value(), TestComponents::Velocity{3.25F}));
    EXPECT(scene.add_component<Storm::Math::Transform1D>(
        second_child.value(),
        Storm::Math::Transform1D{.position = 123.45678901234567}));
    EXPECT(scene.add_component<Storm::Math::Transform3D>(
        root.value(),
        Storm::Math::Transform3D{.translation = {2.0F, 4.0F, 6.0F}}));

    const auto archive = SceneSerializer::serialize<
        TestComponents::Position,
        TestComponents::Velocity,
        Storm::Math::Transform1D,
        Storm::Math::Transform3D>(scene);
    EXPECT(archive);

    auto restored_result = SceneSerializer::deserialize<
        TestComponents::Position,
        TestComponents::Velocity,
        Storm::Math::Transform1D,
        Storm::Math::Transform3D>(archive.value());
    EXPECT(restored_result);
    Storm::Scene restored = std::move(restored_result).value();
    EXPECT(restored.name() == "Gameplay");
    EXPECT(restored.entity_count() == 3U);
    EXPECT(restored.component_type_count() == 4U);

    std::array<std::string, 3U> names{};
    std::size_t name_count = 0U;
    const auto iteration = restored.for_each_entity(
        [&](Storm::EntityHandle, std::string_view name) {
            if (name_count < names.size()) {
                names[name_count] = name;
            }
            ++name_count;
        });
    EXPECT(iteration);
    EXPECT(iteration.value() == 3U);
    EXPECT(name_count == 3U);
    EXPECT(names[0U] == "Root");
    EXPECT(names[1U] == "FirstChild");
    EXPECT(names[2U] == "SecondChild");

    const auto root_handle = [&]() -> std::optional<Storm::EntityHandle> {
        auto found = std::optional<Storm::EntityHandle>{};
        static_cast<void>(restored.for_each_entity(
            [&](Storm::EntityHandle handle, std::string_view name) {
                if (name == "Root") found = handle;
            }));
        return found;
    }();
    const auto first_handle = [&]() -> std::optional<Storm::EntityHandle> {
        auto found = std::optional<Storm::EntityHandle>{};
        static_cast<void>(restored.for_each_entity(
            [&](Storm::EntityHandle handle, std::string_view name) {
                if (name == "FirstChild") found = handle;
            }));
        return found;
    }();
    const auto second_handle = [&]() -> std::optional<Storm::EntityHandle> {
        auto found = std::optional<Storm::EntityHandle>{};
        static_cast<void>(restored.for_each_entity(
            [&](Storm::EntityHandle handle, std::string_view name) {
                if (name == "SecondChild") found = handle;
            }));
        return found;
    }();
    EXPECT(root_handle.has_value());
    EXPECT(first_handle.has_value());
    EXPECT(second_handle.has_value());

    const auto children = restored.children_of(*root_handle);
    EXPECT(children);
    EXPECT(children.value().size() == 2U);
    EXPECT(children.value()[0U] == *second_handle);
    EXPECT(children.value()[1U] == *first_handle);

    const auto position = restored.try_get_component<TestComponents::Position>(*first_handle);
    const auto velocity = restored.try_get_component<TestComponents::Velocity>(*second_handle);
    const auto transform = restored.try_get_component<Storm::Math::Transform3D>(*root_handle);
    const auto one_d_transform =
        restored.try_get_component<Storm::Math::Transform1D>(*second_handle);
    EXPECT(position != nullptr);
    EXPECT(velocity != nullptr);
    EXPECT(transform != nullptr);
    EXPECT(one_d_transform != nullptr);
    EXPECT(one_d_transform->position == 123.45678901234567);
    EXPECT(std::fabs(position->x - 12.5F) < 1.0e-5F);
    EXPECT(std::fabs(position->y + 4.0F) < 1.0e-5F);
    EXPECT(std::fabs(velocity->value - 3.25F) < 1.0e-5F);

    const auto world = Storm::TransformSystem::world_matrix(restored, *root_handle);
    EXPECT(world);
    EXPECT(std::fabs(world.value().at(0U, 3U) - 2.0F) < 1.0e-5F);
    EXPECT(std::fabs(world.value().at(1U, 3U) - 4.0F) < 1.0e-5F);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_component_archive_is_canonical_across_codec_order() {
    auto scene_result = Storm::Scene::create("Canonical");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);
    EXPECT(scene.add_component<TestComponents::Position>(
        entity.value(), TestComponents::Position{1.0F, 2.0F}));
    EXPECT(scene.add_component<TestComponents::Velocity>(
        entity.value(), TestComponents::Velocity{3.0F}));

    const auto first = SceneSerializer::serialize<
        TestComponents::Position, TestComponents::Velocity>(scene);
    const auto second = SceneSerializer::serialize<
        TestComponents::Velocity, TestComponents::Position>(scene);
    EXPECT(first);
    EXPECT(second);
    EXPECT(first.value() == second.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_empty_scene_round_trip_requires_no_component_codecs() {
    auto scene_result = Storm::Scene::create("EmptyScene");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();

    const auto archive = SceneSerializer::serialize<>(scene);
    EXPECT(archive);
    const auto restored = SceneSerializer::deserialize<>(archive.value());
    EXPECT(restored);
    EXPECT(restored.value().name() == "EmptyScene");
    EXPECT(restored.value().entity_count() == 0U);
    EXPECT(restored.value().component_type_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_unregistered_component_types_are_rejected() {
    auto scene_result = Storm::Scene::create("Unregistered");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);
    EXPECT(scene.add_component<TestComponents::Velocity>(
        entity.value(), TestComponents::Velocity{2.0F}));

    const auto archive = SceneSerializer::serialize<TestComponents::Position>(scene);
    EXPECT(!archive);
    EXPECT(archive.error().code == Storm::ErrorCode::Unsupported);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_magic_version_truncation_and_trailing_bytes_are_rejected() {
    auto scene_result = Storm::Scene::create("ArchiveChecks");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto archive_result = SceneSerializer::serialize<>(scene);
    EXPECT(archive_result);
    const auto original = archive_result.value();

    auto invalid_magic = original;
    invalid_magic[0U] = std::byte{0U};
    EXPECT(!SceneSerializer::deserialize<>(invalid_magic));

    auto invalid_version = original;
    invalid_version[4U] = std::byte{2U};
    EXPECT(!SceneSerializer::deserialize<>(invalid_version));

    auto truncated = original;
    truncated.pop_back();
    EXPECT(!SceneSerializer::deserialize<>(truncated));

    auto trailing = original;
    trailing.push_back(std::byte{0U});
    EXPECT(!SceneSerializer::deserialize<>(trailing));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_archive_limits_are_enforced() {
    auto scene_result = Storm::Scene::create("LimitScene");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("AnEntity");
    EXPECT(entity);

    SceneArchiveLimits byte_limit{};
    byte_limit.maximum_archive_bytes = 8U;
    const auto too_large = SceneSerializer::serialize<>(scene, byte_limit);
    EXPECT(!too_large);
    EXPECT(too_large.error().code == Storm::ErrorCode::ResourceExhausted);

    SceneArchiveLimits entity_limit{};
    entity_limit.maximum_entities = 0U;
    const auto too_many_entities = SceneSerializer::serialize<>(scene, entity_limit);
    EXPECT(!too_many_entities);
    EXPECT(too_many_entities.error().code == Storm::ErrorCode::ResourceExhausted);

    SceneArchiveLimits name_limit{};
    name_limit.maximum_string_bytes = 2U;
    const auto long_name = SceneSerializer::serialize<>(scene, name_limit);
    EXPECT(!long_name);
    EXPECT(long_name.error().code == Storm::ErrorCode::ResourceExhausted);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_unknown_component_type_is_not_silently_dropped() {
    auto scene_result = Storm::Scene::create("UnknownType");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);
    EXPECT(scene.add_component<TestComponents::Position>(
        entity.value(), TestComponents::Position{1.0F, 2.0F}));

    auto archive = SceneSerializer::serialize<TestComponents::Position>(scene);
    EXPECT(archive);
    EXPECT(replace_ascii(archive.value(), "test.position", "test.unknown!"));

    const auto restored =
        SceneSerializer::deserialize<TestComponents::Position>(archive.value());
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::Unsupported);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_parent_reference_is_rejected() {
    const auto archive = make_raw_archive(
        "InvalidParent",
        {
            RawEntity{"A", 7U, 0U},
        },
        {});
    EXPECT(archive.has_value());
    const auto restored = SceneSerializer::deserialize<>(*archive);
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_parent_cycles_are_rejected_transactionally() {
    const auto archive = make_raw_archive(
        "Cycle",
        {
            RawEntity{"A", 1U, 0U},
            RawEntity{"B", 0U, 0U},
        },
        {});
    EXPECT(archive.has_value());
    const auto restored = SceneSerializer::deserialize<>(*archive);
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_duplicate_component_records_are_rejected() {
    const auto archive = make_raw_archive(
        "DuplicateRecords",
        {
            RawEntity{"A"},
            RawEntity{"B"},
        },
        {
            RawBlock{
                "test.position",
                1U,
                {
                    RawRecord{0U, position_payload(1.0F, 2.0F)},
                    RawRecord{0U, position_payload(3.0F, 4.0F)},
                },
            },
        });
    EXPECT(archive.has_value());
    const auto restored =
        SceneSerializer::deserialize<TestComponents::Position>(*archive);
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_component_schema_versions_are_checked() {
    const auto archive = make_raw_archive(
        "VersionedComponent",
        {
            RawEntity{"A"},
        },
        {
            RawBlock{
                "test.position",
                22U,
                {RawRecord{0U, position_payload(1.0F, 2.0F)}},
            },
        });
    EXPECT(archive.has_value());
    const auto restored =
        SceneSerializer::deserialize<TestComponents::Position>(*archive);
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::Unsupported);

    BinaryWriter transform1d_payload(8U);
    EXPECT(transform1d_payload.write_f64(12.5));
    const auto transform1d_archive = make_raw_archive(
        "VersionedTransform1D",
        {RawEntity{"A"}},
        {
            RawBlock{
                "storm.math.transform1d",
                22U,
                {RawRecord{0U, std::move(transform1d_payload).take()}},
            },
        });
    EXPECT(transform1d_archive.has_value());
    const auto unsupported_transform1d =
        SceneSerializer::deserialize<Storm::Math::Transform1D>(*transform1d_archive);
    EXPECT(!unsupported_transform1d);
    EXPECT(unsupported_transform1d.error().code == Storm::ErrorCode::Unsupported);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_transform_values_are_rejected() {
    BinaryWriter payload_writer(128U);
    const std::array<float, 10U> invalid_fields{
        0.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 0.0F,
        1.0F, 1.0F, 1.0F,
    };
    for (const float value : invalid_fields) {
        EXPECT(payload_writer.write_f32(value));
    }

    const auto archive = make_raw_archive(
        "InvalidTransform",
        {RawEntity{"A"}},
        {
            RawBlock{
                "storm.math.transform3d",
                1U,
                {RawRecord{0U, std::move(payload_writer).take()}},
            },
        });
    EXPECT(archive.has_value());
    const auto restored =
        SceneSerializer::deserialize<Storm::Math::Transform3D>(*archive);
    EXPECT(!restored);
    EXPECT(restored.error().code == Storm::ErrorCode::InvalidArgument);

    BinaryWriter invalid_transform1d_payload(8U);
    EXPECT(invalid_transform1d_payload.write_f64(
        std::numeric_limits<double>::quiet_NaN()));
    const auto invalid_transform1d_archive = make_raw_archive(
        "InvalidTransform1D",
        {RawEntity{"A"}},
        {
            RawBlock{
                "storm.math.transform1d",
                1U,
                {RawRecord{0U, std::move(invalid_transform1d_payload).take()}},
            },
        });
    EXPECT(invalid_transform1d_archive.has_value());
    const auto invalid_transform1d =
        SceneSerializer::deserialize<Storm::Math::Transform1D>(*invalid_transform1d_archive);
    EXPECT(!invalid_transform1d);
    EXPECT(invalid_transform1d.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_binary_string_operations_are_transactional() {
    BinaryWriter too_small(4U);
    const auto rejected_write = too_small.write_string("A", 1U);
    EXPECT(!rejected_write);
    EXPECT(too_small.size() == 0U);

    BinaryWriter exact_fit(5U);
    const auto accepted_write = exact_fit.write_string("A", 1U);
    EXPECT(accepted_write);
    EXPECT(exact_fit.size() == 5U);

    const std::array<std::byte, 5U> over_limit_data{
        std::byte{3U}, std::byte{0U}, std::byte{0U}, std::byte{0U}, std::byte{'A'},
    };
    Storm::Serialization::BinaryReader over_limit_reader(over_limit_data);
    const auto over_limit_read = over_limit_reader.read_string(2U);
    EXPECT(!over_limit_read);
    EXPECT(over_limit_reader.remaining() == over_limit_data.size());

    Storm::Serialization::BinaryReader truncated_reader(over_limit_data);
    const auto truncated_read = truncated_reader.read_string(3U);
    EXPECT(!truncated_read);
    EXPECT(truncated_reader.remaining() == over_limit_data.size());

    BinaryWriter too_small_double(7U);
    const auto rejected_double = too_small_double.write_f64(123.45678901234567);
    EXPECT(!rejected_double);
    EXPECT(too_small_double.size() == 0U);

    BinaryWriter exact_double(8U);
    constexpr double expected_double = 123.45678901234567;
    EXPECT(exact_double.write_f64(expected_double));
    const auto double_bytes = std::move(exact_double).take();
    EXPECT(double_bytes.size() == 8U);

    Storm::Serialization::BinaryReader double_reader(double_bytes);
    const auto decoded_double = double_reader.read_f64();
    EXPECT(decoded_double);
    EXPECT(decoded_double.value() == expected_double);
    EXPECT(double_reader.empty());

    std::vector<std::byte> truncated_double_bytes(
        double_bytes.begin(), double_bytes.end() - 1);
    Storm::Serialization::BinaryReader truncated_double_reader(truncated_double_bytes);
    const auto rejected_truncated_double = truncated_double_reader.read_f64();
    EXPECT(!rejected_truncated_double);
    EXPECT(truncated_double_reader.remaining() == 7U);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{
            "round_trip_preserves_hierarchy_components_and_transforms",
            &test_round_trip_preserves_hierarchy_components_and_transforms},
        TestCase{
            "component_archive_is_canonical_across_codec_order",
            &test_component_archive_is_canonical_across_codec_order},
        TestCase{
            "empty_scene_round_trip_requires_no_component_codecs",
            &test_empty_scene_round_trip_requires_no_component_codecs},
        TestCase{
            "unregistered_component_types_are_rejected",
            &test_unregistered_component_types_are_rejected},
        TestCase{
            "invalid_magic_version_truncation_and_trailing_bytes_are_rejected",
            &test_invalid_magic_version_truncation_and_trailing_bytes_are_rejected},
        TestCase{
            "archive_limits_are_enforced",
            &test_archive_limits_are_enforced},
        TestCase{
            "unknown_component_type_is_not_silently_dropped",
            &test_unknown_component_type_is_not_silently_dropped},
        TestCase{
            "invalid_parent_reference_is_rejected",
            &test_invalid_parent_reference_is_rejected},
        TestCase{
            "parent_cycles_are_rejected_transactionally",
            &test_parent_cycles_are_rejected_transactionally},
        TestCase{
            "duplicate_component_records_are_rejected",
            &test_duplicate_component_records_are_rejected},
        TestCase{
            "component_schema_versions_are_checked",
            &test_component_schema_versions_are_checked},
        TestCase{
            "invalid_transform_values_are_rejected",
            &test_invalid_transform_values_are_rejected},
        TestCase{
            "binary_string_operations_are_transactional",
            &test_binary_string_operations_are_transactional},
    };

    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto failure = test.run();
        if (failure.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *failure << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }

    std::cout << "Executed " << tests.size()
              << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
