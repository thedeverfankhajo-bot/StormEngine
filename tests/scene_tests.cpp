#include "storm/core/scene.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
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

struct Position final {
    int x{0};
    int y{0};
};

struct Velocity final {
    float x{0.0F};
    float y{0.0F};
};

[[nodiscard]] std::optional<std::string> test_scene_validation_and_identity() {
    auto invalid = Storm::Scene::create(" \t\n");
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result);
    EXPECT(second_result);

    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();
    EXPECT(first.id() != 0U);
    EXPECT(first.id() != second.id());
    EXPECT(first.name() == "First");
    EXPECT(second.name() == "Second");
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_create_name_and_entity_count() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();

    auto player = scene.create_entity("Player");
    auto unnamed = scene.create_entity();
    EXPECT(player);
    EXPECT(unnamed);
    EXPECT(scene.entity_count() == 2U);

    auto name = scene.entity_name(player.value());
    EXPECT(name);
    EXPECT(name.value() == "Player");

    auto rename = scene.rename_entity(unnamed.value(), "Camera");
    EXPECT(rename);
    auto renamed = scene.entity_name(unnamed.value());
    EXPECT(renamed);
    EXPECT(renamed.value() == "Camera");
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_rejects_foreign_handles() {
    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();

    auto entity = first.create_entity("OwnedByFirst");
    EXPECT(entity);
    EXPECT(!second.is_alive(entity.value()));
    EXPECT(!second.destroy_entity(entity.value()));
    EXPECT(second.entity_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroy_invalidates_handle_and_reuse_changes_generation() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();

    auto original = scene.create_entity("Original");
    EXPECT(original);
    const Storm::EntityHandle stale = original.value();

    auto destroyed = scene.destroy_entity(stale);
    EXPECT(destroyed);
    EXPECT(!scene.is_alive(stale));
    EXPECT(scene.entity_count() == 0U);
    EXPECT(!scene.entity_name(stale));

    auto replacement = scene.create_entity("Replacement");
    EXPECT(replacement);
    EXPECT(replacement.value().index == stale.index);
    EXPECT(replacement.value().generation != stale.generation);
    EXPECT(!scene.is_alive(stale));
    EXPECT(scene.is_alive(replacement.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_component_storage_is_typed_and_scene_local() {
    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();

    auto entity = first.create_entity("Player");
    EXPECT(entity);

    auto position = first.add_component<Position>(entity.value(), Position{4, 9});
    EXPECT(position);
    EXPECT(position.value().get().x == 4);
    EXPECT(position.value().get().y == 9);

    auto velocity = first.add_component<Velocity>(entity.value(), 1.5F, -2.0F);
    EXPECT(velocity);
    EXPECT(velocity.value().get().x == 1.5F);
    EXPECT(velocity.value().get().y == -2.0F);

    Position* stored_position = first.try_get_component<Position>(entity.value());
    EXPECT(stored_position != nullptr);
    stored_position->x = 12;
    EXPECT(first.try_get_component<Position>(entity.value())->x == 12);
    EXPECT(second.try_get_component<Position>(entity.value()) == nullptr);

    const Storm::Scene& const_first = first;
    const Position* const_position =
        const_first.try_get_component<Position>(entity.value());
    EXPECT(const_position != nullptr);
    EXPECT(const_position->x == 12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_duplicate_and_missing_components_report_errors() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);

    auto added = scene.add_component<Position>(entity.value(), Position{1, 2});
    EXPECT(added);

    auto duplicate = scene.add_component<Position>(entity.value(), Position{3, 4});
    EXPECT(!duplicate);
    EXPECT(duplicate.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(scene.try_get_component<Position>(entity.value())->x == 1);

    auto missing = scene.remove_component<Velocity>(entity.value());
    EXPECT(!missing);
    EXPECT(missing.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_component_remove_and_entity_destroy_release_storage() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);

    auto position = scene.add_component<Position>(entity.value(), Position{2, 3});
    auto velocity = scene.add_component<Velocity>(entity.value(), Velocity{4.0F, 5.0F});
    EXPECT(position);
    EXPECT(velocity);

    auto removed = scene.remove_component<Position>(entity.value());
    EXPECT(removed);
    EXPECT(scene.try_get_component<Position>(entity.value()) == nullptr);
    EXPECT(scene.try_get_component<Velocity>(entity.value()) != nullptr);

    auto destroyed = scene.destroy_entity(entity.value());
    EXPECT(destroyed);
    EXPECT(scene.try_get_component<Velocity>(entity.value()) == nullptr);
    EXPECT(scene.entity_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_move_preserves_handles_and_invalidates_source() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto source = std::move(result).value();
    auto entity = source.create_entity("Player");
    EXPECT(entity);
    const Storm::EntityHandle handle = entity.value();

    Storm::Scene destination(std::move(source));
    EXPECT(destination.is_alive(handle));
    EXPECT(destination.entity_name(handle));
    EXPECT(!source.is_alive(handle));
    EXPECT(!source.create_entity("Invalid"));
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"scene_validation_and_identity", &test_scene_validation_and_identity},
        TestCase{"create_name_and_entity_count", &test_create_name_and_entity_count},
        TestCase{"scene_rejects_foreign_handles", &test_scene_rejects_foreign_handles},
        TestCase{"destroy_invalidates_handle_and_reuse_changes_generation", &test_destroy_invalidates_handle_and_reuse_changes_generation},
        TestCase{"component_storage_is_typed_and_scene_local", &test_component_storage_is_typed_and_scene_local},
        TestCase{"duplicate_and_missing_components_report_errors", &test_duplicate_and_missing_components_report_errors},
        TestCase{"component_remove_and_entity_destroy_release_storage", &test_component_remove_and_entity_destroy_release_storage},
        TestCase{"move_preserves_handles_and_invalidates_source", &test_move_preserves_handles_and_invalidates_source},
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
