#include "storm/core/transform_system.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] bool nearly_equal(float lhs, float rhs, float tolerance = 1.0e-4F) {
    return std::fabs(lhs - rhs) <= tolerance;
}

[[nodiscard]] std::optional<std::string> test_missing_component_is_identity() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);

    const auto local = Storm::TransformSystem::local_transform(scene, entity.value());
    const auto world = Storm::TransformSystem::world_matrix(scene, entity.value());
    EXPECT(local);
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 0U), 1.0F));
    EXPECT(nearly_equal(world.value().at(1U, 1U), 1.0F));
    EXPECT(nearly_equal(world.value().at(2U, 2U), 1.0F));
    EXPECT(nearly_equal(world.value().at(3U, 3U), 1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_set_and_read_local_transform() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);

    const Storm::Math::Transform3D transform{
        .translation = {3.0F, 4.0F, 5.0F},
        .scale = {2.0F, 2.0F, 2.0F},
    };
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, entity.value(), transform));
    const auto local = Storm::TransformSystem::local_transform(scene, entity.value());
    EXPECT(local);
    EXPECT(nearly_equal(local.value().translation.x, 3.0F));
    EXPECT(nearly_equal(local.value().translation.y, 4.0F));
    EXPECT(nearly_equal(local.value().scale.z, 2.0F));

    const Storm::Math::Transform3D replacement{
        .translation = {8.0F, 0.0F, 0.0F},
    };
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, entity.value(), replacement));
    const auto updated = Storm::TransformSystem::local_transform(scene, entity.value());
    EXPECT(updated);
    EXPECT(nearly_equal(updated.value().translation.x, 8.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_stale_and_foreign_handles_are_rejected() {
    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();
    auto first_entity = first.create_entity("FirstEntity");
    auto foreign_entity = second.create_entity("ForeignEntity");
    EXPECT(first_entity);
    EXPECT(foreign_entity);

    const auto foreign = Storm::TransformSystem::set_local_transform(
        first, foreign_entity.value(), Storm::Math::Transform3D{});
    EXPECT(!foreign);
    EXPECT(foreign.error().code == Storm::ErrorCode::InvalidState);

    const Storm::EntityHandle stale = first_entity.value();
    EXPECT(first.destroy_entity(stale));
    const auto stale_world = Storm::TransformSystem::world_matrix(first, stale);
    EXPECT(!stale_world);
    EXPECT(stale_world.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_transform_values_are_rejected() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);

    const Storm::Math::Transform3D invalid{
        .translation = {std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
    };
    const auto set_result = Storm::TransformSystem::set_local_transform(
        scene, entity.value(), invalid);
    EXPECT(!set_result);
    EXPECT(set_result.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(scene.try_get_component<Storm::Math::Transform3D>(entity.value()) == nullptr);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_local_transform_produces_local_world_matrix() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene,
        entity.value(),
        Storm::Math::Transform3D{.translation = {1.0F, 2.0F, 3.0F}}));

    const auto world = Storm::TransformSystem::world_matrix(scene, entity.value());
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), 1.0F));
    EXPECT(nearly_equal(world.value().at(1U, 3U), 2.0F));
    EXPECT(nearly_equal(world.value().at(2U, 3U), 3.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_parent_and_child_translation_compose() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));

    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, parent.value(),
        Storm::Math::Transform3D{.translation = {10.0F, 0.0F, 0.0F}}));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, child.value(),
        Storm::Math::Transform3D{.translation = {0.0F, 2.0F, 0.0F}}));

    const auto world = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), 10.0F));
    EXPECT(nearly_equal(world.value().at(1U, 3U), 2.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_parent_rotation_and_scale_affect_child() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));

    const auto quarter_turn = Storm::Math::Quaternion::from_axis_angle(
        Storm::Math::Vector3{0.0F, 0.0F, 1.0F}, 1.57079632679F);
    EXPECT(quarter_turn);
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene,
        parent.value(),
        Storm::Math::Transform3D{
            .translation = {10.0F, 0.0F, 0.0F},
            .rotation = quarter_turn.value(),
            .scale = {2.0F, 1.0F, 1.0F},
        }));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene,
        child.value(),
        Storm::Math::Transform3D{.translation = {1.0F, 0.0F, 0.0F}}));

    const auto world = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), 10.0F));
    EXPECT(nearly_equal(world.value().at(1U, 3U), 2.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_parent_without_transform_is_identity() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("GroupingNode");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, child.value(),
        Storm::Math::Transform3D{.translation = {4.0F, 5.0F, 6.0F}}));

    const auto world = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), 4.0F));
    EXPECT(nearly_equal(world.value().at(1U, 3U), 5.0F));
    EXPECT(nearly_equal(world.value().at(2U, 3U), 6.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroying_parent_orphans_child_transform() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, parent.value(),
        Storm::Math::Transform3D{.translation = {10.0F, 0.0F, 0.0F}}));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene, child.value(),
        Storm::Math::Transform3D{.translation = {2.0F, 0.0F, 0.0F}}));

    const auto before = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(before);
    EXPECT(nearly_equal(before.value().at(0U, 3U), 12.0F));

    EXPECT(scene.destroy_entity(parent.value()));
    const auto after = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(after);
    EXPECT(nearly_equal(after.value().at(0U, 3U), 2.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_deep_hierarchy_is_evaluated_iteratively() {
    auto scene_result = Storm::Scene::create("DeepHierarchy");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    constexpr std::size_t depth = 512U;

    std::optional<Storm::EntityHandle> parent;
    Storm::EntityHandle leaf{};
    for (std::size_t index = 0U; index < depth; ++index) {
        auto entity = scene.create_entity();
        EXPECT(entity);
        leaf = entity.value();
        if (parent.has_value()) {
            EXPECT(scene.set_parent(leaf, *parent));
        }
        EXPECT(Storm::TransformSystem::set_local_transform(
            scene, leaf,
            Storm::Math::Transform3D{.translation = {1.0F, 0.0F, 0.0F}}));
        parent = leaf;
    }

    const auto world = Storm::TransformSystem::world_matrix(scene, leaf);
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), static_cast<float>(depth)));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_stored_component_is_reported() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Entity");
    EXPECT(entity);

    const Storm::Math::Transform3D invalid{
        .rotation = Storm::Math::Quaternion{0.0F, 0.0F, 0.0F, 0.0F},
    };
    const auto inserted =
        scene.add_component<Storm::Math::Transform3D>(entity.value(), invalid);
    EXPECT(inserted);

    const auto local = Storm::TransformSystem::local_transform(scene, entity.value());
    const auto world = Storm::TransformSystem::world_matrix(scene, entity.value());
    EXPECT(!local);
    EXPECT(!world);
    EXPECT(local.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(world.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_move_preserves_world_transform() {
    auto scene_result = Storm::Scene::create("Gameplay");
    EXPECT(scene_result);
    auto source = std::move(scene_result).value();
    auto parent = source.create_entity("Parent");
    auto child = source.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(source.set_parent(child.value(), parent.value()));
    EXPECT(Storm::TransformSystem::set_local_transform(
        source, parent.value(),
        Storm::Math::Transform3D{.translation = {7.0F, 0.0F, 0.0F}}));
    EXPECT(Storm::TransformSystem::set_local_transform(
        source, child.value(),
        Storm::Math::Transform3D{.translation = {0.0F, 3.0F, 0.0F}}));

    const Storm::EntityHandle child_handle = child.value();
    Storm::Scene destination(std::move(source));
    const auto world = Storm::TransformSystem::world_matrix(destination, child_handle);
    EXPECT(world);
    EXPECT(nearly_equal(world.value().at(0U, 3U), 7.0F));
    EXPECT(nearly_equal(world.value().at(1U, 3U), 3.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_world_matrix_rejects_hierarchy_scale_overflow() {
    auto scene_result = Storm::Scene::create("OverflowingHierarchy");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));

    EXPECT(Storm::TransformSystem::set_local_transform(
        scene,
        parent.value(),
        Storm::Math::Transform3D{
            .scale = {std::numeric_limits<float>::max(), 1.0F, 1.0F},
        }));
    EXPECT(Storm::TransformSystem::set_local_transform(
        scene,
        child.value(),
        Storm::Math::Transform3D{
            .scale = {2.0F, 1.0F, 1.0F},
        }));

    const auto world = Storm::TransformSystem::world_matrix(scene, child.value());
    EXPECT(!world);
    EXPECT(world.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"missing_component_is_identity", &test_missing_component_is_identity},
        TestCase{"set_and_read_local_transform", &test_set_and_read_local_transform},
        TestCase{"stale_and_foreign_handles_are_rejected", &test_stale_and_foreign_handles_are_rejected},
        TestCase{"invalid_transform_values_are_rejected", &test_invalid_transform_values_are_rejected},
        TestCase{"local_transform_produces_local_world_matrix", &test_local_transform_produces_local_world_matrix},
        TestCase{"parent_and_child_translation_compose", &test_parent_and_child_translation_compose},
        TestCase{"parent_rotation_and_scale_affect_child", &test_parent_rotation_and_scale_affect_child},
        TestCase{"parent_without_transform_is_identity", &test_parent_without_transform_is_identity},
        TestCase{"destroying_parent_orphans_child_transform", &test_destroying_parent_orphans_child_transform},
        TestCase{"deep_hierarchy_is_evaluated_iteratively", &test_deep_hierarchy_is_evaluated_iteratively},
        TestCase{"invalid_stored_component_is_reported", &test_invalid_stored_component_is_reported},
        TestCase{"scene_move_preserves_world_transform", &test_scene_move_preserves_world_transform},
        TestCase{"world_matrix_rejects_hierarchy_scale_overflow", &test_world_matrix_rejects_hierarchy_scale_overflow},
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
