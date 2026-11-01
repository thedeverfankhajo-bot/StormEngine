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

[[nodiscard]] std::optional<std::string> test_set_parent_and_read_hierarchy() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);

    EXPECT(scene.set_parent(child.value(), parent.value()));
    auto child_parent = scene.parent_of(child.value());
    auto parent_children = scene.children_of(parent.value());
    EXPECT(child_parent);
    EXPECT(child_parent.value().has_value());
    EXPECT(*child_parent.value() == parent.value());
    EXPECT(parent_children);
    EXPECT(parent_children.value().size() == 1U);
    EXPECT(parent_children.value()[0] == child.value());

    auto root_parent = scene.parent_of(parent.value());
    EXPECT(root_parent);
    EXPECT(!root_parent.value().has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_reparent_updates_both_sides_and_is_idempotent() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto first_parent = scene.create_entity("First");
    auto second_parent = scene.create_entity("Second");
    auto child = scene.create_entity("Child");
    EXPECT(first_parent);
    EXPECT(second_parent);
    EXPECT(child);

    EXPECT(scene.set_parent(child.value(), first_parent.value()));
    EXPECT(scene.set_parent(child.value(), second_parent.value()));
    EXPECT(scene.set_parent(child.value(), second_parent.value()));

    auto first_children = scene.children_of(first_parent.value());
    auto second_children = scene.children_of(second_parent.value());
    auto child_parent = scene.parent_of(child.value());
    EXPECT(first_children);
    EXPECT(first_children.value().empty());
    EXPECT(second_children);
    EXPECT(second_children.value().size() == 1U);
    EXPECT(second_children.value()[0] == child.value());
    EXPECT(child_parent);
    EXPECT(child_parent.value() == std::optional<Storm::EntityHandle>{second_parent.value()});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_detach_preserves_remaining_sibling_order() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto parent = scene.create_entity("Parent");
    auto first = scene.create_entity("First");
    auto second = scene.create_entity("Second");
    auto third = scene.create_entity("Third");
    EXPECT(parent);
    EXPECT(first);
    EXPECT(second);
    EXPECT(third);

    EXPECT(scene.set_parent(first.value(), parent.value()));
    EXPECT(scene.set_parent(second.value(), parent.value()));
    EXPECT(scene.set_parent(third.value(), parent.value()));
    EXPECT(scene.set_parent(second.value(), std::nullopt));

    auto children = scene.children_of(parent.value());
    auto detached_parent = scene.parent_of(second.value());
    EXPECT(children);
    EXPECT(children.value().size() == 2U);
    EXPECT(children.value()[0] == first.value());
    EXPECT(children.value()[1] == third.value());
    EXPECT(detached_parent);
    EXPECT(!detached_parent.value().has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_reject_self_parent_and_hierarchy_cycles() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto root = scene.create_entity("Root");
    auto child = scene.create_entity("Child");
    auto grandchild = scene.create_entity("Grandchild");
    EXPECT(root);
    EXPECT(child);
    EXPECT(grandchild);

    auto self = scene.set_parent(root.value(), root.value());
    EXPECT(!self);
    EXPECT(self.error().code == Storm::ErrorCode::InvalidArgument);

    EXPECT(scene.set_parent(child.value(), root.value()));
    EXPECT(scene.set_parent(grandchild.value(), child.value()));
    auto cycle = scene.set_parent(root.value(), grandchild.value());
    EXPECT(!cycle);
    EXPECT(cycle.error().code == Storm::ErrorCode::InvalidArgument);

    auto root_parent = scene.parent_of(root.value());
    auto child_parent = scene.parent_of(child.value());
    auto grandchild_parent = scene.parent_of(grandchild.value());
    EXPECT(root_parent);
    EXPECT(!root_parent.value().has_value());
    EXPECT(child_parent);
    EXPECT(child_parent.value() == std::optional<Storm::EntityHandle>{root.value()});
    EXPECT(grandchild_parent);
    EXPECT(grandchild_parent.value() == std::optional<Storm::EntityHandle>{child.value()});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_reject_foreign_and_stale_parent_handles() {
    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();

    auto child = first.create_entity("Child");
    auto foreign_parent = second.create_entity("ForeignParent");
    EXPECT(child);
    EXPECT(foreign_parent);
    auto foreign = first.set_parent(child.value(), foreign_parent.value());
    EXPECT(!foreign);
    EXPECT(foreign.error().code == Storm::ErrorCode::InvalidState);

    auto stale_parent = first.create_entity("StaleParent");
    EXPECT(stale_parent);
    const Storm::EntityHandle stale = stale_parent.value();
    EXPECT(first.destroy_entity(stale));
    auto stale_result = first.set_parent(child.value(), stale);
    EXPECT(!stale_result);
    EXPECT(stale_result.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroying_parent_orphans_children_and_preserves_subtrees() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    auto grandchild = scene.create_entity("Grandchild");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(grandchild);

    EXPECT(scene.set_parent(child.value(), parent.value()));
    EXPECT(scene.set_parent(grandchild.value(), child.value()));
    EXPECT(scene.destroy_entity(parent.value()));

    auto child_parent = scene.parent_of(child.value());
    auto grandchild_parent = scene.parent_of(grandchild.value());
    auto child_children = scene.children_of(child.value());
    EXPECT(child_parent);
    EXPECT(!child_parent.value().has_value());
    EXPECT(grandchild_parent);
    EXPECT(grandchild_parent.value() == std::optional<Storm::EntityHandle>{child.value()});
    EXPECT(child_children);
    EXPECT(child_children.value().size() == 1U);
    EXPECT(child_children.value()[0] == grandchild.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroying_child_removes_parent_link_before_slot_reuse() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto scene = std::move(result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(scene.set_parent(child.value(), parent.value()));

    const Storm::EntityHandle old_child = child.value();
    EXPECT(scene.destroy_entity(old_child));
    auto empty_children = scene.children_of(parent.value());
    EXPECT(empty_children);
    EXPECT(empty_children.value().empty());

    auto replacement = scene.create_entity("Replacement");
    EXPECT(replacement);
    EXPECT(replacement.value().index == old_child.index);
    EXPECT(replacement.value().generation != old_child.generation);
    auto replacement_parent = scene.parent_of(replacement.value());
    EXPECT(replacement_parent);
    EXPECT(!replacement_parent.value().has_value());
    auto children_after_reuse = scene.children_of(parent.value());
    EXPECT(children_after_reuse);
    EXPECT(children_after_reuse.value().empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_move_preserves_hierarchy_handles() {
    auto result = Storm::Scene::create("Gameplay");
    EXPECT(result);
    auto source = std::move(result).value();
    auto parent = source.create_entity("Parent");
    auto child = source.create_entity("Child");
    EXPECT(parent);
    EXPECT(child);
    EXPECT(source.set_parent(child.value(), parent.value()));

    const Storm::EntityHandle parent_handle = parent.value();
    const Storm::EntityHandle child_handle = child.value();
    Storm::Scene destination(std::move(source));

    EXPECT(destination.is_alive(parent_handle));
    EXPECT(destination.is_alive(child_handle));
    auto actual_parent = destination.parent_of(child_handle);
    auto actual_children = destination.children_of(parent_handle);
    EXPECT(actual_parent);
    EXPECT(actual_parent.value() == std::optional<Storm::EntityHandle>{parent_handle});
    EXPECT(actual_children);
    EXPECT(actual_children.value().size() == 1U);
    EXPECT(actual_children.value()[0] == child_handle);
    EXPECT(!source.is_alive(child_handle));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_entity_iteration_is_ordered_and_skips_destroyed_slots() {
    auto result = Storm::Scene::create("Iteration");
    EXPECT(result);
    auto scene = std::move(result).value();

    auto first = scene.create_entity("First");
    auto removed = scene.create_entity("Removed");
    auto third = scene.create_entity("Third");
    EXPECT(first);
    EXPECT(removed);
    EXPECT(third);
    EXPECT(scene.destroy_entity(removed.value()));

    auto replacement = scene.create_entity("Replacement");
    EXPECT(replacement);
    EXPECT(replacement.value().index == removed.value().index);

    std::array<Storm::EntityHandle, 3U> handles{};
    std::array<std::string, 3U> names{};
    std::size_t callback_count = 0U;
    const auto iteration = scene.for_each_entity(
        [&](Storm::EntityHandle handle, std::string_view name) {
            if (callback_count < handles.size()) {
                handles[callback_count] = handle;
                names[callback_count] = name;
            }
            ++callback_count;
        });

    EXPECT(iteration);
    EXPECT(iteration.value() == 3U);
    EXPECT(callback_count == 3U);
    EXPECT(handles[0U] == first.value());
    EXPECT(handles[1U] == replacement.value());
    EXPECT(handles[2U] == third.value());
    EXPECT(names[0U] == "First");
    EXPECT(names[1U] == "Replacement");
    EXPECT(names[2U] == "Third");

    Storm::Scene moved(std::move(scene));
    const auto moved_from_iteration = scene.for_each_entity(
        [](Storm::EntityHandle, std::string_view) {});
    EXPECT(!moved_from_iteration);
    EXPECT(moved_from_iteration.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(moved.is_alive(first.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_component_iteration_is_ordered_typed_and_const_safe() {
    auto result = Storm::Scene::create("ComponentIteration");
    EXPECT(result);
    auto scene = std::move(result).value();

    auto first = scene.create_entity("First");
    auto second = scene.create_entity("Second");
    auto third = scene.create_entity("Third");
    EXPECT(first);
    EXPECT(second);
    EXPECT(third);

    EXPECT(scene.add_component<Position>(first.value(), Position{1, 10}));
    EXPECT(scene.add_component<Position>(third.value(), Position{3, 30}));
    EXPECT(scene.destroy_entity(first.value()));

    auto replacement = scene.create_entity("Replacement");
    EXPECT(replacement);
    EXPECT(replacement.value().index == first.value().index);
    EXPECT(scene.add_component<Position>(replacement.value(), Position{7, 70}));

    std::array<Storm::EntityHandle, 2U> mutable_handles{};
    std::array<int, 2U> mutable_values{};
    std::size_t mutable_callbacks = 0U;
    const auto mutable_iteration = scene.for_each_component<Position>(
        [&](Storm::EntityHandle handle, Position& position) {
            if (mutable_callbacks < mutable_handles.size()) {
                mutable_handles[mutable_callbacks] = handle;
                mutable_values[mutable_callbacks] = position.x;
            }
            ++mutable_callbacks;
            ++position.x;
        });

    EXPECT(mutable_iteration);
    EXPECT(mutable_iteration.value() == 2U);
    EXPECT(mutable_callbacks == 2U);
    EXPECT(mutable_handles[0U] == replacement.value());
    EXPECT(mutable_handles[1U] == third.value());
    EXPECT(mutable_values[0U] == 7);
    EXPECT(mutable_values[1U] == 3);

    const Storm::Scene& const_scene = scene;
    std::size_t const_callbacks = 0U;
    const auto const_iteration = const_scene.for_each_component<Position>(
        [&](Storm::EntityHandle, const Position& position) {
            EXPECT(position.x == 8 || position.x == 4);
            ++const_callbacks;
        });
    EXPECT(const_iteration);
    EXPECT(const_iteration.value() == 2U);
    EXPECT(const_callbacks == 2U);

    std::size_t missing_callbacks = 0U;
    const auto missing_iteration = scene.for_each_component<Velocity>(
        [&](Storm::EntityHandle, Velocity&) {
            ++missing_callbacks;
        });
    EXPECT(missing_iteration);
    EXPECT(missing_iteration.value() == 0U);
    EXPECT(missing_callbacks == 0U);
    EXPECT(scene.is_alive(second.value()));
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
        TestCase{"set_parent_and_read_hierarchy", &test_set_parent_and_read_hierarchy},
        TestCase{"reparent_updates_both_sides_and_is_idempotent", &test_reparent_updates_both_sides_and_is_idempotent},
        TestCase{"detach_preserves_remaining_sibling_order", &test_detach_preserves_remaining_sibling_order},
        TestCase{"reject_self_parent_and_hierarchy_cycles", &test_reject_self_parent_and_hierarchy_cycles},
        TestCase{"reject_foreign_and_stale_parent_handles", &test_reject_foreign_and_stale_parent_handles},
        TestCase{"destroying_parent_orphans_children_and_preserves_subtrees", &test_destroying_parent_orphans_children_and_preserves_subtrees},
        TestCase{"destroying_child_removes_parent_link_before_slot_reuse", &test_destroying_child_removes_parent_link_before_slot_reuse},
        TestCase{"move_preserves_hierarchy_handles", &test_move_preserves_hierarchy_handles},
        TestCase{"entity_iteration_is_ordered_and_skips_destroyed_slots", &test_entity_iteration_is_ordered_and_skips_destroyed_slots},
        TestCase{"component_iteration_is_ordered_typed_and_const_safe", &test_component_iteration_is_ordered_typed_and_const_safe},
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
