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


[[nodiscard]] std::optional<std::string> test_overlap_query_returns_hits_in_stable_slot_order() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{.position = 0.75});
    auto second = world.create_body(BodyDefinition1D{.position = 0.0});
    auto outside = world.create_body(BodyDefinition1D{.position = 3.0});
    EXPECT(first);
    EXPECT(second);
    EXPECT(outside);

    const auto hits = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .position = 0.4,
        .half_extent = 0.25,
    });
    EXPECT(hits);
    EXPECT(hits.value().size() == 2U);
    EXPECT(hits.value()[0] == first.value());
    EXPECT(hits.value()[1] == second.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_filters_layers_and_triggers() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto layer_one = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .collision_layer = 0x1U,
    });
    auto trigger = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .is_trigger = true,
        .collision_layer = 0x2U,
    });
    auto layer_two = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .collision_layer = 0x2U,
    });
    EXPECT(layer_one);
    EXPECT(trigger);
    EXPECT(layer_two);

    const auto with_triggers = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .collision_mask = 0x2U,
    });
    EXPECT(with_triggers);
    EXPECT(with_triggers.value().size() == 2U);
    EXPECT(with_triggers.value()[0] == trigger.value());
    EXPECT(with_triggers.value()[1] == layer_two.value());

    const auto without_triggers = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .collision_mask = 0x2U,
        .include_triggers = false,
    });
    EXPECT(without_triggers);
    EXPECT(without_triggers.value().size() == 1U);
    EXPECT(without_triggers.value()[0] == layer_two.value());

    const auto other_layer = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .collision_mask = 0x1U,
    });
    EXPECT(other_layer);
    EXPECT(other_layer.value().size() == 1U);
    EXPECT(other_layer.value()[0] == layer_one.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_includes_touching_point_bounds() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{
        .position = 1.0,
        .half_extent = 0.5,
    });
    EXPECT(body);

    const auto hits = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .position = 0.5,
        .half_extent = 0.0,
    });
    EXPECT(hits);
    EXPECT(hits.value().size() == 1U);
    EXPECT(hits.value()[0] == body.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_rejects_invalid_input_and_moved_from_world() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto invalid = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .position = std::numeric_limits<double>::quiet_NaN(),
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .half_extent = -1.0,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.overlap_query(Storm::Physics::OneD::OverlapQuery1D{
        .position = std::numeric_limits<double>::max(),
        .half_extent = std::numeric_limits<double>::max(),
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    PhysicsWorld1D moved(std::move(world));
    const auto moved_from_query = world.overlap_query({});
    EXPECT(!moved_from_query);
    EXPECT(moved_from_query.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}


[[nodiscard]] std::optional<std::string> test_raycast_returns_nearest_positive_hit_and_stable_tie() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto farther = world.create_body(BodyDefinition1D{.position = 8.0});
    auto nearest = world.create_body(BodyDefinition1D{.position = 3.0});
    auto tied = world.create_body(BodyDefinition1D{.position = 3.0});
    EXPECT(farther);
    EXPECT(nearest);
    EXPECT(tied);

    const auto result = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 10.0,
    });
    EXPECT(result);
    EXPECT(result.value().has_value());
    EXPECT(result.value()->body == nearest.value());
    EXPECT(result.value()->distance == 2.5);
    EXPECT(result.value()->point == 2.5);
    EXPECT(result.value()->normal == -1.0);
    EXPECT(!result.value()->is_trigger);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_supports_negative_direction() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto farther = world.create_body(BodyDefinition1D{.position = -8.0});
    auto nearest = world.create_body(BodyDefinition1D{.position = -3.0});
    EXPECT(farther);
    EXPECT(nearest);

    const auto result = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = -1.0,
        .maximum_distance = 10.0,
    });
    EXPECT(result);
    EXPECT(result.value().has_value());
    EXPECT(result.value()->body == nearest.value());
    EXPECT(result.value()->distance == 2.5);
    EXPECT(result.value()->point == -2.5);
    EXPECT(result.value()->normal == 1.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_filters_layers_and_triggers() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto filtered_layer = world.create_body(BodyDefinition1D{
        .position = 1.0,
        .collision_layer = 0x1U,
    });
    auto trigger = world.create_body(BodyDefinition1D{
        .position = 3.0,
        .is_trigger = true,
        .collision_layer = 0x2U,
    });
    auto solid = world.create_body(BodyDefinition1D{
        .position = 5.0,
        .collision_layer = 0x2U,
    });
    EXPECT(filtered_layer);
    EXPECT(trigger);
    EXPECT(solid);

    const auto with_triggers = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 10.0,
        .collision_mask = 0x2U,
    });
    EXPECT(with_triggers);
    EXPECT(with_triggers.value().has_value());
    EXPECT(with_triggers.value()->body == trigger.value());
    EXPECT(with_triggers.value()->is_trigger);

    const auto without_triggers = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 10.0,
        .collision_mask = 0x2U,
        .include_triggers = false,
    });
    EXPECT(without_triggers);
    EXPECT(without_triggers.value().has_value());
    EXPECT(without_triggers.value()->body == solid.value());
    EXPECT(!without_triggers.value()->is_trigger);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_reports_zero_distance_when_origin_is_inside_or_on_boundary() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{
        .position = 0.0,
        .half_extent = 0.5,
    });
    EXPECT(body);

    const auto inside = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.1,
        .direction = -1.0,
        .maximum_distance = 0.0,
    });
    EXPECT(inside);
    EXPECT(inside.value().has_value());
    EXPECT(inside.value()->body == body.value());
    EXPECT(inside.value()->distance == 0.0);
    EXPECT(inside.value()->point == 0.1);
    EXPECT(inside.value()->normal == 1.0);

    const auto boundary = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.5,
        .direction = 1.0,
        .maximum_distance = 0.0,
    });
    EXPECT(boundary);
    EXPECT(boundary.value().has_value());
    EXPECT(boundary.value()->distance == 0.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_maximum_distance_is_inclusive() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition1D{
        .position = 3.0,
        .half_extent = 0.5,
    });
    EXPECT(body);

    const auto exact_limit = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 2.5,
    });
    EXPECT(exact_limit);
    EXPECT(exact_limit.value().has_value());
    EXPECT(exact_limit.value()->body == body.value());
    EXPECT(exact_limit.value()->distance == 2.5);

    const auto before_limit = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 2.49,
    });
    EXPECT(before_limit);
    EXPECT(!before_limit.value().has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_rejects_invalid_queries_and_body_bounds() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .direction = 0.0,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .direction = 2.0,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = std::numeric_limits<double>::quiet_NaN(),
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .maximum_distance = -1.0,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .maximum_distance = std::numeric_limits<double>::infinity(),
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = std::numeric_limits<double>::max(),
        .direction = 1.0,
        .maximum_distance = std::numeric_limits<double>::max(),
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    auto huge_body = world.create_body(BodyDefinition1D{
        .position = 1.0e308,
        .half_extent = 1.0e308,
    });
    EXPECT(huge_body);
    invalid = world.raycast(Storm::Physics::OneD::RaycastQuery1D{
        .origin = 0.0,
        .direction = 1.0,
        .maximum_distance = 1.0e308,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::ResourceExhausted);

    PhysicsWorld1D moved(std::move(world));
    const auto moved_from = world.raycast({});
    EXPECT(!moved_from);
    EXPECT(moved_from.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contact_events_transition_begin_stay_end() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.0,
    });
    auto second = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(first);
    EXPECT(second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Begin);
    EXPECT(began.value().contact_events[0].contact.first == first.value());
    EXPECT(began.value().contact_events[0].contact.second == second.value());

    auto stayed = world.step(1.0 / 60.0);
    EXPECT(stayed);
    EXPECT(stayed.value().contact_events.size() == 1U);
    EXPECT(stayed.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Stay);

    EXPECT(world.set_position(second.value(), 2.0));
    auto ended = world.step(1.0 / 60.0);
    EXPECT(ended);
    EXPECT(ended.value().contacts.empty());
    EXPECT(ended.value().contact_events.size() == 1U);
    EXPECT(ended.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::End);
    EXPECT(ended.value().contact_events[0].contact.second == second.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_trigger_contact_events_preserve_trigger_metadata() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.0,
    });
    auto trigger = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
        .is_trigger = true,
    });
    EXPECT(first);
    EXPECT(trigger);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Begin);
    EXPECT(began.value().contact_events[0].contact.is_trigger);

    auto stayed = world.step(1.0 / 60.0);
    EXPECT(stayed);
    EXPECT(stayed.value().contact_events.size() == 1U);
    EXPECT(stayed.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Stay);
    EXPECT(stayed.value().contact_events[0].contact.is_trigger);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroy_and_slot_reuse_emit_end_then_begin() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.0,
    });
    auto old_body = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(first);
    EXPECT(old_body);
    EXPECT(world.step(1.0 / 60.0));

    EXPECT(world.destroy_body(old_body.value()));
    auto replacement = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(replacement);
    EXPECT(replacement.value().slot_index == old_body.value().slot_index);
    EXPECT(replacement.value().generation != old_body.value().generation);

    auto transitioned = world.step(1.0 / 60.0);
    EXPECT(transitioned);
    EXPECT(transitioned.value().contact_events.size() == 2U);
    EXPECT(transitioned.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::End);
    EXPECT(transitioned.value().contact_events[0].contact.second == old_body.value());
    EXPECT(transitioned.value().contact_events[1].phase ==
        Storm::Physics::OneD::ContactPhase1D::Begin);
    EXPECT(transitioned.value().contact_events[1].contact.second == replacement.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contact_events_are_deterministic_by_pair_order() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    for (std::size_t index = 0U; index < 3U; ++index) {
        auto body = world.create_body(BodyDefinition1D{
            .type = BodyType1D::Static,
            .position = 0.0,
        });
        EXPECT(body);
    }

    auto stepped = world.step(1.0 / 60.0);
    EXPECT(stepped);
    EXPECT(stepped.value().contact_events.size() == 3U);
    const auto& events = stepped.value().contact_events;
    EXPECT(events[0].contact.first.slot_index == 0U);
    EXPECT(events[0].contact.second.slot_index == 1U);
    EXPECT(events[1].contact.first.slot_index == 0U);
    EXPECT(events[1].contact.second.slot_index == 2U);
    EXPECT(events[2].contact.first.slot_index == 1U);
    EXPECT(events[2].contact.second.slot_index == 2U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_failed_step_does_not_advance_contact_history() {
    auto world_result = PhysicsWorld1D::create(
        PhysicsWorld1DConfig{.maximum_contacts_per_step = 1U});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.0,
    });
    auto second = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(first);
    EXPECT(second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Begin);

    auto third = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.25,
    });
    EXPECT(third);
    auto failed = world.step(1.0 / 60.0);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::ResourceExhausted);

    EXPECT(world.destroy_body(third.value()));
    auto recovered = world.step(1.0 / 60.0);
    EXPECT(recovered);
    EXPECT(recovered.value().contact_events.size() == 1U);
    EXPECT(recovered.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Stay);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_moving_world_preserves_contact_history() {
    auto world_result = PhysicsWorld1D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.0,
    });
    auto second = world.create_body(BodyDefinition1D{
        .type = BodyType1D::Static,
        .position = 0.5,
    });
    EXPECT(first);
    EXPECT(second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Begin);

    PhysicsWorld1D moved(std::move(world));
    auto stayed = moved.step(1.0 / 60.0);
    EXPECT(stayed);
    EXPECT(stayed.value().contact_events.size() == 1U);
    EXPECT(stayed.value().contact_events[0].phase ==
        Storm::Physics::OneD::ContactPhase1D::Stay);

    auto moved_from_step = world.step(1.0 / 60.0);
    EXPECT(!moved_from_step);
    EXPECT(moved_from_step.error().code == Storm::ErrorCode::InvalidState);
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
        TestCase{"overlap_query_returns_hits_in_stable_slot_order", &test_overlap_query_returns_hits_in_stable_slot_order},
        TestCase{"overlap_query_filters_layers_and_triggers", &test_overlap_query_filters_layers_and_triggers},
        TestCase{"overlap_query_includes_touching_point_bounds", &test_overlap_query_includes_touching_point_bounds},
        TestCase{"overlap_query_rejects_invalid_input_and_moved_from_world", &test_overlap_query_rejects_invalid_input_and_moved_from_world},
        TestCase{"raycast_returns_nearest_positive_hit_and_stable_tie", &test_raycast_returns_nearest_positive_hit_and_stable_tie},
        TestCase{"raycast_supports_negative_direction", &test_raycast_supports_negative_direction},
        TestCase{"raycast_filters_layers_and_triggers", &test_raycast_filters_layers_and_triggers},
        TestCase{"raycast_reports_zero_distance_when_origin_is_inside_or_on_boundary", &test_raycast_reports_zero_distance_when_origin_is_inside_or_on_boundary},
        TestCase{"raycast_maximum_distance_is_inclusive", &test_raycast_maximum_distance_is_inclusive},
        TestCase{"raycast_rejects_invalid_queries_and_body_bounds", &test_raycast_rejects_invalid_queries_and_body_bounds},
        TestCase{"contact_events_transition_begin_stay_end", &test_contact_events_transition_begin_stay_end},
        TestCase{"trigger_contact_events_preserve_trigger_metadata", &test_trigger_contact_events_preserve_trigger_metadata},
        TestCase{"destroy_and_slot_reuse_emit_end_then_begin", &test_destroy_and_slot_reuse_emit_end_then_begin},
        TestCase{"contact_events_are_deterministic_by_pair_order", &test_contact_events_are_deterministic_by_pair_order},
        TestCase{"failed_step_does_not_advance_contact_history", &test_failed_step_does_not_advance_contact_history},
        TestCase{"moving_world_preserves_contact_history", &test_moving_world_preserves_contact_history},
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
