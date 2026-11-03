#include "storm/physics/two_d.hpp"

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

using Storm::Physics::TwoD::BodyDefinition2D;
using Storm::Physics::TwoD::BodyHandle2D;
using Storm::Physics::TwoD::BodyType2D;
using Storm::Physics::TwoD::ContactPhase2D;
using Storm::Physics::TwoD::BoxCollider2D;
using Storm::Physics::TwoD::CircleCollider2D;
using Storm::Physics::TwoD::OverlapQuery2D;
using Storm::Physics::TwoD::PhysicsWorld2D;
using Storm::Physics::TwoD::PhysicsWorld2DConfig;
using Storm::Physics::TwoD::RaycastQuery2D;
using Storm::Physics::TwoD::Vector2;

#define EXPECT(...) \
    do { \
        if (!(__VA_ARGS__)) { \
            return std::string("Expectation failed: ") + #__VA_ARGS__; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_config_validation() {
    EXPECT(!PhysicsWorld2D::create(PhysicsWorld2DConfig{.maximum_bodies = 0U}));
    EXPECT(!PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_candidate_pairs = 0U}));
    EXPECT(!PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.broadphase_cell_size = 0.0}));
    EXPECT(!PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_absolute_coordinate =
            std::numeric_limits<double>::infinity()}));
    EXPECT(PhysicsWorld2D::create());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_handles_are_world_scoped_and_generation_checked() {
    auto first_result = PhysicsWorld2D::create();
    auto second_result = PhysicsWorld2D::create();
    EXPECT(first_result);
    EXPECT(second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();

    auto body = first.create_body(BodyDefinition2D{});
    EXPECT(body);
    EXPECT(!second.body_snapshot(body.value()));

    const BodyHandle2D stale = body.value();
    EXPECT(first.destroy_body(stale));
    EXPECT(!first.body_snapshot(stale));

    auto replacement = first.create_body(BodyDefinition2D{});
    EXPECT(replacement);
    EXPECT(replacement.value().slot_index == stale.slot_index);
    EXPECT(replacement.value().generation != stale.generation);
    EXPECT(!first.destroy_body(stale));
    EXPECT(first.body_count() == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_definition_validation() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    BodyDefinition2D definition;
    definition.mass = 0.0;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.restitution = 1.1;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.friction = -0.1;
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.collider = CircleCollider2D{0.0};
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.collider = BoxCollider2D{{1.0, 0.0}};
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.position.x = std::numeric_limits<double>::quiet_NaN();
    EXPECT(!world.create_body(definition));

    definition = BodyDefinition2D{};
    definition.type = BodyType2D::Static;
    definition.velocity.y = 1.0;
    EXPECT(!world.create_body(definition));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_force_impulse_and_fixed_step_integration() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_step_seconds = 1.0});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition2D{
        .collider = CircleCollider2D{0.25},
        .mass = 2.0,
    });
    EXPECT(body);
    EXPECT(world.apply_force(body.value(), Vector2{4.0, 0.0}));
    EXPECT(world.apply_impulse(body.value(), Vector2{0.0, 2.0}));
    EXPECT(world.step(0.5, Vector2{0.0, -10.0}));

    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(std::abs(snapshot.value().velocity.x - 1.0) < 1.0e-12);
    EXPECT(std::abs(snapshot.value().velocity.y - (-4.0)) < 1.0e-12);
    EXPECT(std::abs(snapshot.value().position.x - 0.5) < 1.0e-12);
    EXPECT(std::abs(snapshot.value().position.y - (-2.0)) < 1.0e-12);
    EXPECT(snapshot.value().accumulated_force == Vector2{});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_static_and_kinematic_body_behavior() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_step_seconds = 0.5});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto fixed = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {5.0, 2.0},
        .collider = CircleCollider2D{0.5},
    });
    auto moving = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Kinematic,
        .position = {-1.0, 0.0},
        .velocity = {3.0, 2.0},
        .collider = CircleCollider2D{0.25},
    });
    EXPECT(fixed);
    EXPECT(moving);
    EXPECT(world.step(0.5, Vector2{0.0, -100.0}));

    auto fixed_state = world.body_snapshot(fixed.value());
    auto moving_state = world.body_snapshot(moving.value());
    EXPECT(fixed_state);
    EXPECT(moving_state);
    EXPECT(fixed_state.value().position == Vector2{5.0, 2.0});
    EXPECT(moving_state.value().position == Vector2{0.5, 1.0});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_circle_collision_and_elastic_impulse() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .position = {0.0, 0.0},
        .velocity = {1.0, 0.0},
        .collider = CircleCollider2D{0.5},
        .restitution = 1.0,
        .friction = 0.0,
    });
    auto second = world.create_body(BodyDefinition2D{
        .position = {0.8, 0.0},
        .velocity = {-1.0, 0.0},
        .collider = CircleCollider2D{0.5},
        .restitution = 1.0,
        .friction = 0.0,
    });
    EXPECT(first);
    EXPECT(second);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].normal == Vector2{1.0, 0.0});
    EXPECT(stepped.value().contacts[0].penetration_depth > 0.0);

    auto first_state = world.body_snapshot(first.value());
    auto second_state = world.body_snapshot(second.value());
    EXPECT(first_state);
    EXPECT(second_state);
    EXPECT(std::abs(first_state.value().velocity.x - (-1.0)) < 1.0e-9);
    EXPECT(std::abs(second_state.value().velocity.x - 1.0) < 1.0e-9);
    EXPECT(second_state.value().position.x - first_state.value().position.x >= 0.94);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_axis_aligned_box_collision_resolution() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .position = {0.0, 0.0},
        .velocity = {1.0, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
        .friction = 0.0,
    });
    auto second = world.create_body(BodyDefinition2D{
        .position = {0.8, 0.0},
        .velocity = {-1.0, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
        .friction = 0.0,
    });
    EXPECT(first);
    EXPECT(second);

    auto stepped = world.step(0.01);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].normal == Vector2{1.0, 0.0});
    auto first_state = world.body_snapshot(first.value());
    auto second_state = world.body_snapshot(second.value());
    EXPECT(first_state);
    EXPECT(second_state);
    EXPECT(second_state.value().position.x - first_state.value().position.x >= 0.94);
    EXPECT(std::abs(first_state.value().velocity.x) < 1.0e-9);
    EXPECT(std::abs(second_state.value().velocity.x) < 1.0e-9);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_circle_box_contact_and_inside_case() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto circle = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Dynamic,
        .position = {-0.8, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    auto box = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
    });
    EXPECT(circle);
    EXPECT(box);
    auto outside = world.step(1.0 / 60.0);
    EXPECT(outside);
    EXPECT(outside.value().contacts.size() == 1U);

    auto inner_result = PhysicsWorld2D::create();
    EXPECT(inner_result);
    auto inner_world = std::move(inner_result).value();
    auto inner_circle = inner_world.create_body(BodyDefinition2D{
        .position = {0.0, 0.0},
        .collider = CircleCollider2D{0.25},
    });
    auto inner_box = inner_world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
        .collider = BoxCollider2D{{1.0, 1.0}},
    });
    EXPECT(inner_circle);
    EXPECT(inner_box);
    auto inside = inner_world.step(1.0 / 60.0);
    EXPECT(inside);
    EXPECT(inside.value().contacts.size() == 1U);
    EXPECT(std::isfinite(inside.value().contacts[0].normal.x));
    EXPECT(std::isfinite(inside.value().contacts[0].normal.y));
    EXPECT(std::abs(std::hypot(
        inside.value().contacts[0].normal.x,
        inside.value().contacts[0].normal.y) - 1.0) < 1.0e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_trigger_and_collision_filtering() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto trigger = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .collider = CircleCollider2D{1.0},
        .is_trigger = true,
        .collision_layer = 0x1U,
        .collision_mask = 0x2U,
    });
    auto actor = world.create_body(BodyDefinition2D{
        .position = {0.5, 0.0},
        .collider = CircleCollider2D{0.5},
        .collision_layer = 0x2U,
        .collision_mask = 0x1U,
    });
    EXPECT(trigger);
    EXPECT(actor);
    auto stepped = world.step(1.0 / 60.0);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(stepped.value().contacts[0].is_trigger);

    auto filtered_result = PhysicsWorld2D::create();
    EXPECT(filtered_result);
    auto filtered = std::move(filtered_result).value();
    auto first = filtered.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .collider = CircleCollider2D{1.0},
        .collision_layer = 0x1U,
        .collision_mask = 0x1U,
    });
    auto second = filtered.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
        .collider = CircleCollider2D{1.0},
        .collision_layer = 0x2U,
        .collision_mask = 0x2U,
    });
    EXPECT(first);
    EXPECT(second);
    auto ignored = filtered.step(1.0 / 60.0);
    EXPECT(ignored);
    EXPECT(ignored.value().contacts.empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_friction_reduces_tangential_sliding() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .position = {0.0, 0.0},
        .velocity = {1.0, 1.0},
        .collider = CircleCollider2D{0.5},
        .friction = 1.0,
    });
    auto second = world.create_body(BodyDefinition2D{
        .position = {0.8, 0.0},
        .velocity = {-1.0, 0.0},
        .collider = CircleCollider2D{0.5},
        .friction = 1.0,
    });
    EXPECT(first);
    EXPECT(second);
    EXPECT(world.step(0.001));
    auto first_state = world.body_snapshot(first.value());
    auto second_state = world.body_snapshot(second.value());
    EXPECT(first_state);
    EXPECT(second_state);
    EXPECT(std::abs(
        second_state.value().velocity.y - first_state.value().velocity.y) < 1.0e-9);
    EXPECT(std::abs(first_state.value().velocity.y) < 1.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_spatial_hash_skips_distant_pairs() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
        .collider = CircleCollider2D{0.25},
    });
    auto second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {100.0, 100.0},
        .collider = CircleCollider2D{0.25},
    });
    EXPECT(first);
    EXPECT(second);
    auto stepped = world.step(1.0 / 60.0);
    EXPECT(stepped);
    EXPECT(stepped.value().broadphase_candidate_pairs == 0U);
    EXPECT(stepped.value().contacts.empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_cell_and_candidate_budgets_fail_transactionally() {
    auto cell_world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{
            .maximum_cells_per_body = 1U,
            .broadphase_cell_size = 1.0,
        });
    EXPECT(cell_world_result);
    auto cell_world = std::move(cell_world_result).value();
    auto oversized = cell_world.create_body(BodyDefinition2D{
        .position = {2.0, 3.0},
        .velocity = {1.0, 0.0},
        .collider = BoxCollider2D{{2.0, 2.0}},
    });
    EXPECT(oversized);
    EXPECT(cell_world.apply_force(oversized.value(), Vector2{2.0, 0.0}));
    auto cell_failure = cell_world.step(0.1);
    EXPECT(!cell_failure);
    EXPECT(cell_failure.error().code == Storm::ErrorCode::ResourceExhausted);
    auto unchanged = cell_world.body_snapshot(oversized.value());
    EXPECT(unchanged);
    EXPECT(unchanged.value().position == Vector2{2.0, 3.0});
    EXPECT(unchanged.value().velocity == Vector2{1.0, 0.0});
    EXPECT(unchanged.value().accumulated_force == Vector2{2.0, 0.0});

    auto pair_world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_candidate_pairs = 2U});
    EXPECT(pair_world_result);
    auto pair_world = std::move(pair_world_result).value();
    for (int index = 0; index < 3; ++index) {
        auto body = pair_world.create_body(BodyDefinition2D{
            .type = BodyType2D::Static,
            .position = {0.0, 0.0},
            .collider = CircleCollider2D{0.25},
        });
        EXPECT(body);
    }
    auto pair_failure = pair_world.step(0.1);
    EXPECT(!pair_failure);
    EXPECT(pair_failure.error().code == Storm::ErrorCode::ResourceExhausted);
    EXPECT(pair_world.body_count() == 3U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contact_limit_and_body_capacity() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{
            .maximum_bodies = 2U,
            .maximum_contacts_per_step = 1U,
        });
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .collider = CircleCollider2D{1.0},
    });
    auto second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
        .collider = CircleCollider2D{1.0},
    });
    EXPECT(first);
    EXPECT(second);
    EXPECT(!world.create_body(BodyDefinition2D{}));
    auto stepped = world.step(0.1);
    EXPECT(stepped);
    EXPECT(stepped.value().contacts.size() == 1U);
    EXPECT(world.destroy_body(first.value()));
    EXPECT(world.create_body(BodyDefinition2D{}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_invalid_step_preserves_state() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_step_seconds = 0.1});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition2D{
        .velocity = {2.0, -1.0},
        .collider = CircleCollider2D{0.5},
    });
    EXPECT(body);
    EXPECT(world.apply_force(body.value(), Vector2{4.0, 2.0}));
    EXPECT(!world.step(0.2));
    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(snapshot.value().position == Vector2{});
    EXPECT(snapshot.value().velocity == Vector2{2.0, -1.0});
    EXPECT(snapshot.value().accumulated_force == Vector2{4.0, 2.0});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_coordinate_overflow_rolls_back() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{
            .maximum_step_seconds = 1.0,
            .maximum_absolute_coordinate = 10.0,
        });
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto body = world.create_body(BodyDefinition2D{
        .position = {9.0, 0.0},
        .velocity = {2.0, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    EXPECT(body);
    auto failed = world.step(1.0);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::ResourceExhausted);
    auto snapshot = world.body_snapshot(body.value());
    EXPECT(snapshot);
    EXPECT(snapshot.value().position == Vector2{9.0, 0.0});
    EXPECT(snapshot.value().velocity == Vector2{2.0, 0.0});
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_moved_from_world_is_inert() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto original = std::move(world_result).value();
    PhysicsWorld2D moved(std::move(original));
    EXPECT(moved.world_id() != 0U);
    EXPECT(original.world_id() == 0U);
    EXPECT(!original.step(1.0 / 60.0));
    EXPECT(!original.create_body(BodyDefinition2D{}));
    return std::nullopt;
}


[[nodiscard]] std::optional<std::string> test_overlap_query_supports_circle_and_box_shapes() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto circle = world.create_body(BodyDefinition2D{
        .position = {1.25, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    const auto box = world.create_body(BodyDefinition2D{
        .position = {-1.25, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
    });
    const auto distant = world.create_body(BodyDefinition2D{
        .position = {4.0, 0.0},
        .collider = CircleCollider2D{0.25},
    });
    EXPECT(circle);
    EXPECT(box);
    EXPECT(distant);

    const auto hits = world.overlap_query(OverlapQuery2D{
        .position = {},
        .collider = CircleCollider2D{0.8},
    });
    EXPECT(hits);
    EXPECT(hits.value().size() == 2U);
    EXPECT(hits.value()[0U] == circle.value());
    EXPECT(hits.value()[1U] == box.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_filters_layers_and_triggers() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto trigger = world.create_body(BodyDefinition2D{
        .collider = CircleCollider2D{0.5},
        .is_trigger = true,
        .collision_layer = 0x2U,
    });
    const auto solid = world.create_body(BodyDefinition2D{
        .collider = BoxCollider2D{{0.5, 0.5}},
        .collision_layer = 0x2U,
    });
    const auto other_layer = world.create_body(BodyDefinition2D{
        .collider = CircleCollider2D{0.5},
        .collision_layer = 0x4U,
    });
    EXPECT(trigger);
    EXPECT(solid);
    EXPECT(other_layer);

    const auto all = world.overlap_query(OverlapQuery2D{
        .collider = CircleCollider2D{1.0},
        .collision_mask = 0x2U,
    });
    EXPECT(all);
    EXPECT(all.value().size() == 2U);
    EXPECT(all.value()[0U] == trigger.value());
    EXPECT(all.value()[1U] == solid.value());

    const auto without_triggers = world.overlap_query(OverlapQuery2D{
        .collider = CircleCollider2D{1.0},
        .collision_mask = 0x2U,
        .include_triggers = false,
    });
    EXPECT(without_triggers);
    EXPECT(without_triggers.value().size() == 1U);
    EXPECT(without_triggers.value()[0U] == solid.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_includes_touching_shapes() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto touching = world.create_body(BodyDefinition2D{
        .position = {1.5, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
    });
    EXPECT(touching);

    const auto hits = world.overlap_query(OverlapQuery2D{
        .position = {},
        .collider = CircleCollider2D{1.0},
    });
    EXPECT(hits);
    EXPECT(hits.value().size() == 1U);
    EXPECT(hits.value()[0U] == touching.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_overlap_query_rejects_invalid_input_and_moved_from_world() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_absolute_coordinate = 10.0});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto invalid = world.overlap_query(OverlapQuery2D{
        .position = {std::numeric_limits<double>::quiet_NaN(), 0.0},
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.overlap_query(OverlapQuery2D{
        .collider = CircleCollider2D{0.0},
    });
    EXPECT(!invalid);

    invalid = world.overlap_query(OverlapQuery2D{
        .position = {11.0, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    EXPECT(!invalid);

    PhysicsWorld2D moved(std::move(world));
    const auto moved_from = world.overlap_query({});
    EXPECT(!moved_from);
    EXPECT(moved_from.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_hits_nearest_circle_and_box() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto circle = world.create_body(BodyDefinition2D{
        .position = {0.0, 0.0},
        .collider = CircleCollider2D{1.0},
    });
    const auto box = world.create_body(BodyDefinition2D{
        .position = {4.0, 0.0},
        .collider = BoxCollider2D{{0.5, 0.5}},
    });
    EXPECT(circle);
    EXPECT(box);

    const auto circle_hit = world.raycast(RaycastQuery2D{
        .origin = {-5.0, 0.0},
        .direction = {10.0, 0.0},
        .maximum_distance = 10.0,
    });
    EXPECT(circle_hit);
    EXPECT(circle_hit.value().has_value());
    EXPECT(circle_hit.value()->body == circle.value());
    EXPECT(std::abs(circle_hit.value()->distance - 4.0) < 1.0e-9);
    EXPECT(std::abs(circle_hit.value()->point.x - (-1.0)) < 1.0e-9);
    EXPECT(std::abs(circle_hit.value()->normal.x - (-1.0)) < 1.0e-9);

    const auto box_hit = world.raycast(RaycastQuery2D{
        .origin = {2.0, 0.0},
        .direction = {7.0, 0.0},
        .maximum_distance = 5.0,
    });
    EXPECT(box_hit);
    EXPECT(box_hit.value().has_value());
    EXPECT(box_hit.value()->body == box.value());
    EXPECT(std::abs(box_hit.value()->distance - 1.5) < 1.0e-9);
    EXPECT(std::abs(box_hit.value()->point.x - 3.5) < 1.0e-9);
    EXPECT(std::abs(box_hit.value()->normal.x - (-1.0)) < 1.0e-9);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_supports_negative_direction_and_inside_origin() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto circle = world.create_body(BodyDefinition2D{
        .collider = CircleCollider2D{1.0},
    });
    EXPECT(circle);

    const auto negative = world.raycast(RaycastQuery2D{
        .origin = {5.0, 0.0},
        .direction = {-2.0, 0.0},
        .maximum_distance = 10.0,
    });
    EXPECT(negative);
    EXPECT(negative.value().has_value());
    EXPECT(negative.value()->body == circle.value());
    EXPECT(std::abs(negative.value()->distance - 4.0) < 1.0e-9);
    EXPECT(std::abs(negative.value()->normal.x - 1.0) < 1.0e-9);

    const auto inside = world.raycast(RaycastQuery2D{
        .origin = {},
        .direction = {0.0, 3.0},
        .maximum_distance = 2.0,
    });
    EXPECT(inside);
    EXPECT(inside.value().has_value());
    EXPECT(inside.value()->distance == 0.0);
    EXPECT(inside.value()->normal == Vector2{0.0, -1.0});

    const auto boundary = world.raycast(RaycastQuery2D{
        .origin = {1.0, 0.0},
        .direction = {1.0, 0.0},
        .maximum_distance = 2.0,
    });
    EXPECT(boundary);
    EXPECT(boundary.value().has_value());
    EXPECT(boundary.value()->distance == 0.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_filters_layers_and_triggers_with_inclusive_limit() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto trigger = world.create_body(BodyDefinition2D{
        .position = {2.0, 0.0},
        .collider = CircleCollider2D{0.5},
        .is_trigger = true,
        .collision_layer = 0x2U,
    });
    const auto solid = world.create_body(BodyDefinition2D{
        .position = {4.0, 0.0},
        .collider = CircleCollider2D{0.5},
        .collision_layer = 0x2U,
    });
    const auto other_layer = world.create_body(BodyDefinition2D{
        .position = {1.0, 0.0},
        .collider = CircleCollider2D{0.25},
        .collision_layer = 0x4U,
    });
    EXPECT(trigger);
    EXPECT(solid);
    EXPECT(other_layer);

    const auto exact_limit = world.raycast(RaycastQuery2D{
        .origin = {},
        .direction = {2.0, 0.0},
        .maximum_distance = 1.5,
        .collision_mask = 0x2U,
    });
    EXPECT(exact_limit);
    EXPECT(exact_limit.value().has_value());
    EXPECT(exact_limit.value()->body == trigger.value());
    EXPECT(std::abs(exact_limit.value()->distance - 1.5) < 1.0e-9);

    const auto without_trigger = world.raycast(RaycastQuery2D{
        .origin = {},
        .direction = {1.0, 0.0},
        .maximum_distance = 3.5,
        .collision_mask = 0x2U,
        .include_triggers = false,
    });
    EXPECT(without_trigger);
    EXPECT(without_trigger.value().has_value());
    EXPECT(without_trigger.value()->body == solid.value());
    EXPECT(std::abs(without_trigger.value()->distance - 3.5) < 1.0e-9);

    const auto no_layers = world.raycast(RaycastQuery2D{
        .origin = {},
        .direction = {1.0, 0.0},
        .maximum_distance = 5.0,
        .collision_mask = 0U,
    });
    EXPECT(no_layers);
    EXPECT(!no_layers.value().has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_raycast_tie_order_and_invalid_queries() {
    auto world_result = PhysicsWorld2D::create(
        PhysicsWorld2DConfig{.maximum_absolute_coordinate = 10.0});
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    const auto first = world.create_body(BodyDefinition2D{
        .position = {2.0, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    const auto second = world.create_body(BodyDefinition2D{
        .position = {2.0, 0.0},
        .collider = CircleCollider2D{0.5},
    });
    EXPECT(first);
    EXPECT(second);

    const auto tie = world.raycast(RaycastQuery2D{
        .origin = {},
        .direction = {1.0, 0.0},
        .maximum_distance = 5.0,
    });
    EXPECT(tie);
    EXPECT(tie.value().has_value());
    EXPECT(tie.value()->body == first.value());

    auto invalid = world.raycast(RaycastQuery2D{
        .direction = {},
        .maximum_distance = 5.0,
    });
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);

    invalid = world.raycast(RaycastQuery2D{
        .origin = {std::numeric_limits<double>::quiet_NaN(), 0.0},
        .direction = {1.0, 0.0},
        .maximum_distance = 5.0,
    });
    EXPECT(!invalid);

    invalid = world.raycast(RaycastQuery2D{
        .direction = {1.0, 0.0},
        .maximum_distance = -1.0,
    });
    EXPECT(!invalid);

    invalid = world.raycast(RaycastQuery2D{
        .origin = {9.0, 0.0},
        .direction = {1.0, 0.0},
        .maximum_distance = 2.0,
    });
    EXPECT(!invalid);

    PhysicsWorld2D moved(std::move(world));
    const auto moved_from = world.raycast({});
    EXPECT(!moved_from);
    EXPECT(moved_from.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_contact_events_transition_begin_stay_end() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
    });
    auto second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
    });
    EXPECT(first);
    EXPECT(second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase == ContactPhase2D::Begin);
    EXPECT(began.value().contact_events[0].contact.first == first.value());
    EXPECT(began.value().contact_events[0].contact.second == second.value());

    auto stayed = world.step(1.0 / 60.0);
    EXPECT(stayed);
    EXPECT(stayed.value().contact_events.size() == 1U);
    EXPECT(stayed.value().contact_events[0].phase == ContactPhase2D::Stay);

    EXPECT(world.set_position(second.value(), {2.0, 0.0}));
    auto ended = world.step(1.0 / 60.0);
    EXPECT(ended);
    EXPECT(ended.value().contacts.empty());
    EXPECT(ended.value().contact_events.size() == 1U);
    EXPECT(ended.value().contact_events[0].phase == ContactPhase2D::End);
    EXPECT(ended.value().contact_events[0].contact.second == second.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_trigger_contact_events_preserve_trigger_metadata() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
    });
    auto trigger = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
        .collider = CircleCollider2D{0.5},
        .is_trigger = true,
    });
    EXPECT(first);
    EXPECT(trigger);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase == ContactPhase2D::Begin);
    EXPECT(began.value().contact_events[0].contact.is_trigger);

    auto stayed = world.step(1.0 / 60.0);
    EXPECT(stayed);
    EXPECT(stayed.value().contact_events.size() == 1U);
    EXPECT(stayed.value().contact_events[0].phase == ContactPhase2D::Stay);
    EXPECT(stayed.value().contact_events[0].contact.is_trigger);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroy_and_slot_reuse_emit_end_then_begin() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
    });
    auto old_second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
    });
    EXPECT(first);
    EXPECT(old_second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase == ContactPhase2D::Begin);

    EXPECT(world.destroy_body(old_second.value()));
    auto new_second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
    });
    EXPECT(new_second);
    EXPECT(new_second.value().slot_index == old_second.value().slot_index);
    EXPECT(new_second.value().generation != old_second.value().generation);

    auto transitions = world.step(1.0 / 60.0);
    EXPECT(transitions);
    EXPECT(transitions.value().contact_events.size() == 2U);
    EXPECT(transitions.value().contact_events[0].phase == ContactPhase2D::End);
    EXPECT(transitions.value().contact_events[0].contact.second == old_second.value());
    EXPECT(transitions.value().contact_events[1].phase == ContactPhase2D::Begin);
    EXPECT(transitions.value().contact_events[1].contact.second == new_second.value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_failed_step_does_not_advance_contact_history() {
    auto world_result = PhysicsWorld2D::create();
    EXPECT(world_result);
    auto world = std::move(world_result).value();

    auto first = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.0, 0.0},
    });
    auto second = world.create_body(BodyDefinition2D{
        .type = BodyType2D::Static,
        .position = {0.5, 0.0},
    });
    EXPECT(first);
    EXPECT(second);

    auto began = world.step(1.0 / 60.0);
    EXPECT(began);
    EXPECT(began.value().contact_events.size() == 1U);
    EXPECT(began.value().contact_events[0].phase == ContactPhase2D::Begin);

    const auto failed = world.step(0.0);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::InvalidArgument);

    auto retried = world.step(1.0 / 60.0);
    EXPECT(retried);
    EXPECT(retried.value().contact_events.size() == 1U);
    EXPECT(retried.value().contact_events[0].phase == ContactPhase2D::Stay);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"config_validation", &test_config_validation},
        TestCase{"handles_are_world_scoped_and_generation_checked", &test_handles_are_world_scoped_and_generation_checked},
        TestCase{"definition_validation", &test_definition_validation},
        TestCase{"force_impulse_and_fixed_step_integration", &test_force_impulse_and_fixed_step_integration},
        TestCase{"static_and_kinematic_body_behavior", &test_static_and_kinematic_body_behavior},
        TestCase{"circle_collision_and_elastic_impulse", &test_circle_collision_and_elastic_impulse},
        TestCase{"axis_aligned_box_collision_resolution", &test_axis_aligned_box_collision_resolution},
        TestCase{"circle_box_contact_and_inside_case", &test_circle_box_contact_and_inside_case},
        TestCase{"trigger_and_collision_filtering", &test_trigger_and_collision_filtering},
        TestCase{"friction_reduces_tangential_sliding", &test_friction_reduces_tangential_sliding},
        TestCase{"spatial_hash_skips_distant_pairs", &test_spatial_hash_skips_distant_pairs},
        TestCase{"cell_and_candidate_budgets_fail_transactionally", &test_cell_and_candidate_budgets_fail_transactionally},
        TestCase{"contact_limit_and_body_capacity", &test_contact_limit_and_body_capacity},
        TestCase{"invalid_step_preserves_state", &test_invalid_step_preserves_state},
        TestCase{"coordinate_overflow_rolls_back", &test_coordinate_overflow_rolls_back},
        TestCase{"moved_from_world_is_inert", &test_moved_from_world_is_inert},
        TestCase{"overlap_query_supports_circle_and_box_shapes", &test_overlap_query_supports_circle_and_box_shapes},
        TestCase{"overlap_query_filters_layers_and_triggers", &test_overlap_query_filters_layers_and_triggers},
        TestCase{"overlap_query_includes_touching_shapes", &test_overlap_query_includes_touching_shapes},
        TestCase{"overlap_query_rejects_invalid_input_and_moved_from_world", &test_overlap_query_rejects_invalid_input_and_moved_from_world},
        TestCase{"raycast_hits_nearest_circle_and_box", &test_raycast_hits_nearest_circle_and_box},
        TestCase{"raycast_supports_negative_direction_and_inside_origin", &test_raycast_supports_negative_direction_and_inside_origin},
        TestCase{"raycast_filters_layers_and_triggers_with_inclusive_limit", &test_raycast_filters_layers_and_triggers_with_inclusive_limit},
        TestCase{"raycast_tie_order_and_invalid_queries", &test_raycast_tie_order_and_invalid_queries},
        TestCase{"contact_events_transition_begin_stay_end", &test_contact_events_transition_begin_stay_end},
        TestCase{"trigger_contact_events_preserve_trigger_metadata", &test_trigger_contact_events_preserve_trigger_metadata},
        TestCase{"destroy_and_slot_reuse_emit_end_then_begin", &test_destroy_and_slot_reuse_emit_end_then_begin},
        TestCase{"failed_step_does_not_advance_contact_history", &test_failed_step_does_not_advance_contact_history},
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
