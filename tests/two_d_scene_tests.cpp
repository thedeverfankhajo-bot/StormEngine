#include "storm/core/scene.hpp"
#include "storm/math/transform2d.hpp"
#include "storm/physics/two_d_scene.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

using Storm::Math::Transform2D;
using Storm::Physics::TwoD::BodyDefinition2D;
using Storm::Physics::TwoD::PhysicsScene2D;
using Storm::Physics::TwoD::Vector2;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_attach_requires_valid_transform() {
    auto scene_result = Storm::Scene::create("Attach");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();

    auto missing = physics.attach(scene, entity.value());
    EXPECT(!missing);
    EXPECT(missing.error().code == Storm::ErrorCode::InvalidState);

    Transform2D invalid;
    invalid.translation.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT(scene.add_component<Transform2D>(entity.value(), invalid));
    auto rejected = physics.attach(scene, entity.value());
    EXPECT(!rejected);
    EXPECT(rejected.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(physics.body_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_position_is_authoritative() {
    auto scene_result = Storm::Scene::create("Position");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);

    Transform2D transform;
    transform.translation = {7.5F, -2.25F};
    EXPECT(scene.add_component<Transform2D>(entity.value(), transform));
    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();

    BodyDefinition2D definition;
    definition.position = {-900.0, 100.0};
    auto body = physics.attach(scene, entity.value(), definition);
    EXPECT(body);
    auto state = physics.body_snapshot(scene, entity.value());
    EXPECT(state);
    EXPECT(std::abs(state.value().position.x - 7.5) < 1e-12);
    EXPECT(std::abs(state.value().position.y + 2.25) < 1e-12);
    EXPECT(!physics.attach(scene, entity.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_fixed_step_syncs_translation_only() {
    auto scene_result = Storm::Scene::create("Step");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);

    Transform2D transform;
    transform.translation = {2.0F, 3.0F};
    transform.rotation_radians = 1.25F;
    transform.scale = {2.0F, 3.0F};
    EXPECT(scene.add_component<Transform2D>(entity.value(), transform));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    BodyDefinition2D definition;
    definition.velocity = {4.0, 0.0};
    EXPECT(physics.attach(scene, entity.value(), definition));

    auto stepped = physics.step(scene, 0.25, Vector2{0.0, -8.0});
    EXPECT(stepped);
    const auto* updated = scene.try_get_component<Transform2D>(entity.value());
    EXPECT(updated != nullptr);
    EXPECT(std::abs(updated->translation.x - 3.0F) < 1e-5F);
    EXPECT(std::abs(updated->translation.y - 2.5F) < 1e-5F);
    EXPECT(std::abs(updated->rotation_radians - 1.25F) < 1e-6F);
    EXPECT(updated->scale.x == 2.0F && updated->scale.y == 3.0F);

    auto body = physics.body_snapshot(scene, entity.value());
    EXPECT(body);
    EXPECT(std::abs(body.value().velocity.x - 4.0) < 1e-12);
    EXPECT(std::abs(body.value().velocity.y + 2.0) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_position_velocity_force_and_impulse() {
    auto scene_result = Storm::Scene::create("Commands");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform2D>(entity.value()));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    BodyDefinition2D definition;
    definition.mass = 2.0;
    EXPECT(physics.attach(scene, entity.value(), definition));
    EXPECT((physics.set_position(scene, entity.value(), Vector2{5.0, 6.0})));
    EXPECT((physics.set_velocity(scene, entity.value(), Vector2{1.0, 2.0})));
    EXPECT((physics.apply_impulse(scene, entity.value(), Vector2{2.0, 0.0})));
    EXPECT((physics.apply_force(scene, entity.value(), Vector2{0.0, 4.0})));

    auto stepped = physics.step(scene, 0.5);
    if (!stepped) {
        return std::string("PhysicsWorld2D step failed: ") + stepped.error().message;
    }
    auto body = physics.body_snapshot(scene, entity.value());
    EXPECT(body);
    EXPECT(std::abs(body.value().velocity.x - 2.0) < 1e-12);
    EXPECT(std::abs(body.value().velocity.y - 3.0) < 1e-12);
    EXPECT(std::abs(body.value().position.x - 6.0) < 1e-12);
    EXPECT(std::abs(body.value().position.y - 7.5) < 1e-12);

    const auto* updated = scene.try_get_component<Transform2D>(entity.value());
    EXPECT(updated != nullptr);
    EXPECT(std::abs(updated->translation.x - 6.0F) < 1e-5F);
    EXPECT(std::abs(updated->translation.y - 7.5F) < 1e-5F);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_lookup_and_detach() {
    auto scene_result = Storm::Scene::create("Lookup");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Object");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform2D>(entity.value()));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    auto body = physics.attach(scene, entity.value());
    EXPECT(body);

    auto body_lookup = physics.body_for_entity(scene, entity.value());
    auto entity_lookup = physics.entity_for_body(scene, body.value());
    EXPECT(body_lookup && body_lookup.value().has_value());
    EXPECT(*body_lookup.value() == body.value());
    EXPECT(entity_lookup && entity_lookup.value().has_value());
    EXPECT(*entity_lookup.value() == entity.value());

    EXPECT(physics.detach(scene, entity.value()));
    EXPECT(physics.body_count() == 0U);
    EXPECT(!physics.detach(scene, entity.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_removed_entities_and_components_are_cleaned() {
    auto scene_result = Storm::Scene::create("Cleanup");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto destroyed = scene.create_entity("Destroyed");
    auto removed = scene.create_entity("ComponentRemoved");
    EXPECT(destroyed && removed);
    EXPECT(scene.add_component<Transform2D>(destroyed.value()));
    EXPECT(scene.add_component<Transform2D>(removed.value()));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    EXPECT(physics.attach(scene, destroyed.value()));
    EXPECT(physics.attach(scene, removed.value()));
    EXPECT(scene.destroy_entity(destroyed.value()));
    EXPECT(scene.remove_component<Transform2D>(removed.value()));

    EXPECT(physics.step(scene, 0.1));
    EXPECT(physics.body_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_foreign_scene_and_stale_entity_rejected() {
    auto first_result = Storm::Scene::create("First");
    auto second_result = Storm::Scene::create("Second");
    EXPECT(first_result && second_result);
    auto first = std::move(first_result).value();
    auto second = std::move(second_result).value();
    auto first_entity = first.create_entity("FirstEntity");
    auto second_entity = second.create_entity("SecondEntity");
    EXPECT(first_entity && second_entity);
    EXPECT(first.add_component<Transform2D>(first_entity.value()));
    EXPECT(second.add_component<Transform2D>(second_entity.value()));

    auto physics_result = PhysicsScene2D::create(first);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    EXPECT(!physics.attach(second, second_entity.value()));
    EXPECT(!physics.attach(first, second_entity.value()));
    EXPECT(first.destroy_entity(first_entity.value()));
    EXPECT(!physics.attach(first, first_entity.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_failed_step_preserves_scene_and_body() {
    auto scene_result = Storm::Scene::create("Failure");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto entity = scene.create_entity("Object");
    EXPECT(entity);
    Transform2D transform;
    transform.translation = {12.0F, -4.0F};
    EXPECT(scene.add_component<Transform2D>(entity.value(), transform));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    EXPECT(physics.attach(scene, entity.value()));
    EXPECT((physics.apply_force(scene, entity.value(), Vector2{3.0, -2.0})));

    auto failure = physics.step(scene, 0.0);
    EXPECT(!failure);
    EXPECT(failure.error().code == Storm::ErrorCode::InvalidArgument);
    const auto* stored = scene.try_get_component<Transform2D>(entity.value());
    EXPECT(stored != nullptr);
    EXPECT(stored->translation.x == 12.0F && stored->translation.y == -4.0F);

    auto state = physics.body_snapshot(scene, entity.value());
    EXPECT(state);
    EXPECT((state.value().position == Vector2{12.0, -4.0}));
    EXPECT((state.value().accumulated_force == Vector2{3.0, -2.0}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_parented_entities_are_rejected_safely() {
    auto scene_result = Storm::Scene::create("Hierarchy");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    auto parent = scene.create_entity("Parent");
    auto child = scene.create_entity("Child");
    EXPECT(parent && child);
    EXPECT(scene.add_component<Transform2D>(parent.value()));
    EXPECT(scene.add_component<Transform2D>(child.value()));
    EXPECT(scene.set_parent(child.value(), parent.value()));

    auto physics_result = PhysicsScene2D::create(scene);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    EXPECT(!physics.attach(scene, child.value()));
    EXPECT(physics.body_count() == 0U);

    EXPECT(scene.set_parent(child.value(), std::nullopt));
    EXPECT(physics.attach(scene, child.value()));
    EXPECT(scene.set_parent(child.value(), parent.value()));
    auto step = physics.step(scene, 0.1);
    EXPECT(!step);
    EXPECT(physics.body_count() == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_and_adapter_moves() {
    auto scene_result = Storm::Scene::create("Moves");
    EXPECT(scene_result);
    auto source = std::move(scene_result).value();
    auto entity = source.create_entity("Player");
    EXPECT(entity);
    Transform2D transform;
    transform.translation = {4.0F, 2.0F};
    EXPECT(source.add_component<Transform2D>(entity.value(), transform));

    auto physics_result = PhysicsScene2D::create(source);
    EXPECT(physics_result);
    auto physics = std::move(physics_result).value();
    Storm::Scene destination(std::move(source));
    PhysicsScene2D moved(std::move(physics));

    EXPECT(destination.is_alive(entity.value()));
    auto moved_position = moved.set_position(destination, entity.value(), Vector2{3.0, 4.0});
    if (!moved_position) {
        return std::string("PhysicsScene2D set_position after move failed: ") +
            moved_position.error().message;
    }
    EXPECT(!physics.attach(destination, entity.value()));
    auto state = moved.body_snapshot(destination, entity.value());
    EXPECT(state);
    EXPECT((state.value().position == Vector2{3.0, 4.0}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_coordinate_limit_fits_transform_storage() {
    auto scene_result = Storm::Scene::create("Range");
    EXPECT(scene_result);
    auto scene = std::move(scene_result).value();
    Storm::Physics::TwoD::PhysicsWorld2DConfig config;
    config.maximum_absolute_coordinate =
        static_cast<double>(std::numeric_limits<float>::max()) * 2.0;
    auto invalid = PhysicsScene2D::create(scene, config);
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"attach_requires_valid_transform", &test_attach_requires_valid_transform},
        TestCase{"scene_position_is_authoritative", &test_scene_position_is_authoritative},
        TestCase{"fixed_step_syncs_translation_only", &test_fixed_step_syncs_translation_only},
        TestCase{"position_velocity_force_and_impulse", &test_position_velocity_force_and_impulse},
        TestCase{"lookup_and_detach", &test_lookup_and_detach},
        TestCase{"removed_entities_and_components_are_cleaned", &test_removed_entities_and_components_are_cleaned},
        TestCase{"foreign_scene_and_stale_entity_rejected", &test_foreign_scene_and_stale_entity_rejected},
        TestCase{"failed_step_preserves_scene_and_body", &test_failed_step_preserves_scene_and_body},
        TestCase{"parented_entities_are_rejected_safely", &test_parented_entities_are_rejected_safely},
        TestCase{"scene_and_adapter_moves", &test_scene_and_adapter_moves},
        TestCase{"coordinate_limit_fits_transform_storage", &test_coordinate_limit_fits_transform_storage},
    };

    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto failure = test.run();
        if (failure) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *failure << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }
    std::cout << "Executed " << tests.size() << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
