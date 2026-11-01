#include "storm/physics/one_d.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

using Storm::Physics::OneD::BodyDefinition1D;
using Storm::Physics::OneD::BodyHandle1D;
using Storm::Physics::OneD::BodyType1D;
using Storm::Physics::OneD::PhysicsWorld1D;
using Storm::Physics::OneD::PhysicsWorld1DConfig;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_config_validation_and_unique_world_ids() {
    auto zero_bodies = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_bodies = 0U});
    EXPECT(!zero_bodies);
    EXPECT(zero_bodies.error().code == Storm::ErrorCode::InvalidArgument);

    auto zero_contacts = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_contacts_per_step = 0U});
    EXPECT(!zero_contacts);

    auto invalid_delta = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_step_seconds =
            std::numeric_limits<double>::infinity()});
    EXPECT(!invalid_delta);

    auto first = PhysicsWorld1D::create();
    auto second = PhysicsWorld1D::create();
    EXPECT(first);
    EXPECT(second);
    EXPECT(first.value().world_id() != 0U);
    EXPECT(first.value().world_id() != second.value().world_id());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_body_definition_validation() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    BodyDefinition1D definition;
    definition.half_extent = 0.0;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition1D{};
    definition.mass = 0.0;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition1D{};
    definition.restitution = 1.1;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition1D{};
    definition.position = std::numeric_limits<double>::quiet_NaN();
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition1D{};
    definition.type = BodyType1D::Static;
    definition.velocity = 1.0;
    EXPECT(!world.create_body(definition));

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_handles_are_world_scoped_and_generation_checked() {
    auto first_result = PhysicsWorld1D::create();
    auto second_result = PhysicsWorld1D::create();
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();

    auto body = first.create_body(BodyDefinition1D{});
    EXPECT(body);
    EXPECT(!second.body_snapshot(body.value()));

    const auto stale = body.value();
    EXPECT(first.destroy_body(stale));
    EXPECT(!first.body_snapshot(stale));

    auto replacement = first.create_body(BodyDefinition1D{});
    EXPECT(replacement);
    EXPECT(replacement.value().slot_index == stale.slot_index);
    EXPECT(replacement.value().generation != stale.generation);
    EXPECT(!first.destroy_body(stale));
    EXPECT(first.body_count() == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_dynamic_body_integrates_gravity_semi_implicitly() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_step_seconds = 1.0});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{});
    EXPECT(body);
    auto stepped = world.step(0.5, -10.0);
    EXPECT(stepped);

    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(std::abs(snapshot.value().velocity - (-5.0)) < 1e-12);
    EXPECT(std::abs(snapshot.value().position - (-2.5)) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_force_is_consumed_by_one_successful_step() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_step_seconds = 1.0});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{
        .mass = 2.0,
    });
    EXPECT(body);
    EXPECT(world.apply_force(body.value(), 10.0));

    EXPECT(world.step(1.0));
    auto after_first = world.body_snapshot(body.value());
    EXPECT(after_first);
    EXPECT(std::abs(after_first.value().velocity - 5.0) < 1e-12);
    EXPECT(after_first.value().accumulated_force == 0.0);

    EXPECT(world.step(1.0));
    auto after_second = world.body_snapshot(body.value());
    EXPECT(after_second);
    EXPECT(std::abs(after_second.value().velocity - 5.0) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_impulse_changes_dynamic_velocity_immediately() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{.mass = 2.0});
    EXPECT(body);
    EXPECT(world.apply_impulse(body.value(), 4.0));

    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(std::abs(snapshot.value().velocity - 2.0) < 1e-12);
    EXPECT(snapshot.value().position == 0.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_kinematic_moves_and_static_remains_fixed() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto fixed = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 10.0,
    });
    auto moving = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Kinematic,
        .position = 0.0,
        .velocity = 3.0,
    });
    EXPECT(fixed);
    EXPECT(moving);

    EXPECT(world.step(0.2));
    auto fixed_state = world.body_snapshot(fixed.value());
    auto moving_state = world.body_snapshot(moving.value());
    EXPECT(fixed_state);
    EXPECT(moving_state);
    EXPECT(fixed_state.value().position == 10.0);
    EXPECT(std::abs(moving_state.value().position - 0.6) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_inelastic_collision_separates_equal_masses() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto left = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .velocity = 1.0,
        .restitution = 0.0,
    });
    auto right = world.create_body(BodyDefinition1D{
        .position = 0.8,
        .velocity = -1.0,
        .restitution = 0.0,
    });
    EXPECT(left);
    EXPECT(right);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].normal == 1.0);
    EXPECT(stepped.value().contacts[0].penetration_depth > 0.0);

    auto left_state = world.body_snapshot(left.value());
    auto right_state = world.body_snapshot(right.value());
    EXPECT(left_state);
    EXPECT(right_state);
    EXPECT(std::abs(left_state.value().velocity) < 1e-12);
    EXPECT(std::abs(right_state.value().velocity) < 1e-12);
    EXPECT(right_state.value().position - left_state.value().position >= 1.0 - 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_elastic_collision_exchanges_equal_mass_velocities() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto left = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .velocity = 1.0,
        .restitution = 1.0,
    });
    auto right = world.create_body(BodyDefinition1D{
        .position = 0.8,
        .velocity = -1.0,
        .restitution = 1.0,
    });
    EXPECT(left);
    EXPECT(right);

    EXPECT(world.step(0.01));
    auto left_state = world.body_snapshot(left.value());
    auto right_state = world.body_snapshot(right.value());
    EXPECT(left_state);
    EXPECT(right_state);
    EXPECT(std::abs(left_state.value().velocity - (-1.0)) < 1e-12);
    EXPECT(std::abs(right_state.value().velocity - 1.0) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_touching_bodies_resolve_closing_velocity() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto left = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .velocity = 1.0,
        .restitution = 0.0,
    });
    auto right = world.create_body(BodyDefinition1D{
        .position = 1.25,
        .velocity = -1.0,
        .restitution = 0.0,
    });
    EXPECT(left);
    EXPECT(right);

    auto stepped = world.step(0.125);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].penetration_depth == 0.0);

    auto left_state = world.body_snapshot(left.value());
    auto right_state = world.body_snapshot(right.value());
    EXPECT(left_state);
    EXPECT(right_state);
    EXPECT(std::abs(left_state.value().velocity) < 1e-12);
    EXPECT(std::abs(right_state.value().velocity) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_trigger_overlap_reports_contact_without_resolution() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto trigger = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .is_trigger = true,
    });
    auto dynamic = world.create_body(BodyDefinition1D{
        .position = 0.8,
        .velocity = -1.0,
    });
    EXPECT(trigger);
    EXPECT(dynamic);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].is_trigger);

    auto trigger_state = world.body_snapshot(trigger.value());
    auto dynamic_state = world.body_snapshot(dynamic.value());
    EXPECT(trigger_state);
    EXPECT(dynamic_state);
    EXPECT(trigger_state.value().position == 0.0);
    EXPECT(std::abs(dynamic_state.value().position - 0.79) < 1e-12);
    EXPECT(dynamic_state.value().velocity == -1.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_collision_masks_filter_pairs() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .collision_layer = 0x1U,
        .collision_mask = 0x1U,
    });
    auto second = world.create_body(BodyDefinition1D{
        .position = 0.5,
        .collision_layer = 0x2U,
        .collision_mask = 0x2U,
    });
    EXPECT(first);
    EXPECT(second);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_body_capacity_is_enforced_and_reusable() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_bodies = 1U});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{});
    EXPECT(first);
    auto exhausted = world.create_body(BodyDefinition1D{});
    EXPECT(!exhausted);
    EXPECT(exhausted.error().code == Storm::ErrorCode::ResourceExhausted);

    EXPECT(world.destroy_body(first.value()));
    auto replacement = world.create_body(BodyDefinition1D{});
    EXPECT(replacement);
    EXPECT(replacement.value().slot_index == first.value().slot_index);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_setters_and_force_reject_invalid_operations() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto fixed = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
    });
    auto dynamic = world.create_body(BodyDefinition1D{});
    EXPECT(fixed);
    EXPECT(dynamic);

    EXPECT(!world.set_position(dynamic.value(), std::numeric_limits<double>::infinity()));
    EXPECT(!world.set_velocity(fixed.value(), 1.0));
    EXPECT(!world.apply_force(fixed.value(), 1.0));
    EXPECT(!world.apply_impulse(fixed.value(), 1.0));
    EXPECT(!world.apply_force(dynamic.value(), std::numeric_limits<double>::quiet_NaN()));
    EXPECT(!world.set_velocity(BodyHandle1D{}, 0.0));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_step_is_transactional() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_step_seconds = 0.1});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{});
    EXPECT(body);
    EXPECT(world.apply_force(body.value(), 5.0));

    EXPECT(!world.step(0.0));
    EXPECT(!world.step(-0.01));
    EXPECT(!world.step(0.2));
    EXPECT(!world.step(0.01, std::numeric_limits<double>::infinity()));

    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(snapshot.value().position == 0.0);
    EXPECT(snapshot.value().velocity == 0.0);
    EXPECT(snapshot.value().accumulated_force == 5.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contact_limit_failure_preserves_all_body_state_and_forces() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{
            .maximum_bodies = 4U,
            .maximum_contacts_per_step = 1U,
            .maximum_step_seconds = 0.1,
        });
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto dynamic = world.create_body(BodyDefinition1D{
        .position = 0.0,
    });
    auto fixed_a = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.25,
    });
    auto fixed_b = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(dynamic);
    EXPECT(fixed_a);
    EXPECT(fixed_b);
    EXPECT(world.apply_force(dynamic.value(), 2.0));

    auto failed_step = world.step(0.05);
    EXPECT(!failed_step);
    EXPECT(failed_step.error().code == Storm::ErrorCode::ResourceExhausted);

    auto state = world.body_snapshot(dynamic.value());
    EXPECT(state);
    EXPECT(state.value().position == 0.0);
    EXPECT(state.value().velocity == 0.0);
    EXPECT(state.value().accumulated_force == 2.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contacts_are_reported_in_stable_slot_order() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{.position = 0.0});
    auto second = world.create_body(BodyDefinition1D{.position = 0.2});
    auto third = world.create_body(BodyDefinition1D{.position = 0.4});
    EXPECT(first);
    EXPECT(second);
    EXPECT(third);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 3U);
    EXPECT(stepped.value().contacts[0].first == first.value());
    EXPECT(stepped.value().contacts[0].second == second.value());
    EXPECT(stepped.value().contacts[1].first == first.value());
    EXPECT(stepped.value().contacts[1].second == third.value());
    EXPECT(stepped.value().contacts[2].first == second.value());
    EXPECT(stepped.value().contacts[2].second == third.value());
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"config_validation_and_unique_world_ids", &test_config_validation_and_unique_world_ids},
        TestCase{"body_definition_validation", &test_body_definition_validation},
        TestCase{"handles_are_world_scoped_and_generation_checked", &test_handles_are_world_scoped_and_generation_checked},
        TestCase{"dynamic_body_integrates_gravity_semi_implicitly", &test_dynamic_body_integrates_gravity_semi_implicitly},
        TestCase{"force_is_consumed_by_one_successful_step", &test_force_is_consumed_by_one_successful_step},
        TestCase{"impulse_changes_dynamic_velocity_immediately", &test_impulse_changes_dynamic_velocity_immediately},
        TestCase{"kinematic_moves_and_static_remains_fixed", &test_kinematic_moves_and_static_remains_fixed},
        TestCase{"inelastic_collision_separates_equal_masses", &test_inelastic_collision_separates_equal_masses},
        TestCase{"elastic_collision_exchanges_equal_mass_velocities", &test_elastic_collision_exchanges_equal_mass_velocities},
        TestCase{"touching_bodies_resolve_closing_velocity", &test_touching_bodies_resolve_closing_velocity},
        TestCase{"trigger_overlap_reports_contact_without_resolution", &test_trigger_overlap_reports_contact_without_resolution},
        TestCase{"collision_masks_filter_pairs", &test_collision_masks_filter_pairs},
        TestCase{"body_capacity_is_enforced_and_reusable", &test_body_capacity_is_enforced_and_reusable},
        TestCase{"setters_and_force_reject_invalid_operations", &test_setters_and_force_reject_invalid_operations},
        TestCase{"invalid_step_is_transactional", &test_invalid_step_is_transactional},
        TestCase{"contact_limit_failure_preserves_all_body_state_and_forces", &test_contact_limit_failure_preserves_all_body_state_and_forces},
        TestCase{"contacts_are_reported_in_stable_slot_order", &test_contacts_are_reported_in_stable_slot_order},
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

    std::cout << "Executed " << std::size(tests)
              << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
