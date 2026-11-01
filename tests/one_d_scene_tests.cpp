#include "storm/core/scene.hpp"
#include "storm/math/transform1d.hpp"
#include "storm/physics/one_d_scene.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

using Storm::Math::Transform1D;
using Storm::Physics::OneD::BodyDefinition1D;
using Storm::Physics::OneD::PhysicsScene1D;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_attach_requires_valid_transform() {
    auto sr = Storm::Scene::create("Attach");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();

    auto missing = physics.attach(scene, entity.value());
    EXPECT(!missing);
    EXPECT(missing.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(scene.add_component<Transform1D>(
        entity.value(), Transform1D{std::numeric_limits<double>::quiet_NaN()}));
    auto invalid = physics.attach(scene, entity.value());
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(physics.body_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_position_is_authoritative() {
    auto sr = Storm::Scene::create("Position");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform1D>(entity.value(), Transform1D{7.5}));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    BodyDefinition1D def;
    def.position = -900.0;
    auto body = physics.attach(scene, entity.value(), def);
    EXPECT(body);
    auto state = physics.body_snapshot(scene, entity.value());
    EXPECT(state);
    EXPECT(std::abs(state.value().position - 7.5) < 1e-12);
    EXPECT(!physics.attach(scene, entity.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_fixed_step_syncs_transform() {
    auto sr = Storm::Scene::create("Step");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform1D>(entity.value(), Transform1D{10.0}));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    EXPECT(physics.attach(scene, entity.value()));
    auto step = physics.step(scene, 0.25, -10.0);
    EXPECT(step);
    const auto* t = scene.try_get_component<Transform1D>(entity.value());
    EXPECT(t != nullptr);
    EXPECT(std::abs(t->position - 9.375) < 1e-12);
    auto body = physics.body_snapshot(scene, entity.value());
    EXPECT(body);
    EXPECT(std::abs(body.value().velocity + 2.5) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_teleport_force_and_impulse() {
    auto sr = Storm::Scene::create("Commands");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Player");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform1D>(entity.value(), Transform1D{1.0}));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    BodyDefinition1D def;
    def.mass = 2.0;
    EXPECT(physics.attach(scene, entity.value(), def));
    EXPECT(physics.set_position(scene, entity.value(), 5.0));
    EXPECT(physics.set_velocity(scene, entity.value(), 1.0));
    EXPECT(physics.apply_impulse(scene, entity.value(), 2.0));
    EXPECT(physics.apply_force(scene, entity.value(), 4.0));
    auto step = physics.step(scene, 0.25, 0.0);
    EXPECT(step);
    auto body = physics.body_snapshot(scene, entity.value());
    EXPECT(body);
    EXPECT(std::abs(body.value().velocity - 2.5) < 1e-12);
    EXPECT(std::abs(body.value().position - 5.625) < 1e-12);
    EXPECT(std::abs(scene.try_get_component<Transform1D>(entity.value())->position - 5.625) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_lookup_and_detach() {
    auto sr = Storm::Scene::create("Lookup");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Object");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform1D>(entity.value()));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
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
    auto sr = Storm::Scene::create("Cleanup");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto a = scene.create_entity("Destroyed");
    auto b = scene.create_entity("ComponentRemoved");
    EXPECT(a && b);
    EXPECT(scene.add_component<Transform1D>(a.value()));
    EXPECT(scene.add_component<Transform1D>(b.value()));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    EXPECT(physics.attach(scene, a.value()));
    EXPECT(physics.attach(scene, b.value()));
    EXPECT(scene.destroy_entity(a.value()));
    EXPECT(scene.remove_component<Transform1D>(b.value()));
    EXPECT(physics.step(scene, 0.1, 0.0));
    EXPECT(physics.body_count() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_foreign_scene_and_stale_handle_rejected() {
    auto ar = Storm::Scene::create("A");
    auto br = Storm::Scene::create("B");
    EXPECT(ar && br);
    auto a = std::move(ar).value();
    auto b = std::move(br).value();
    auto ea = a.create_entity("A");
    auto eb = b.create_entity("B");
    EXPECT(ea && eb);
    EXPECT(a.add_component<Transform1D>(ea.value()));
    EXPECT(b.add_component<Transform1D>(eb.value()));
    auto pr = PhysicsScene1D::create(a);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    EXPECT(!physics.attach(b, eb.value()));
    EXPECT(!physics.attach(a, eb.value()));
    EXPECT(a.destroy_entity(ea.value()));
    EXPECT(!physics.attach(a, ea.value()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_failed_step_preserves_scene_and_body() {
    auto sr = Storm::Scene::create("Failure");
    EXPECT(sr);
    auto scene = std::move(sr).value();
    auto entity = scene.create_entity("Object");
    EXPECT(entity);
    EXPECT(scene.add_component<Transform1D>(entity.value(), Transform1D{12.0}));
    auto pr = PhysicsScene1D::create(scene);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    EXPECT(physics.attach(scene, entity.value()));
    EXPECT(physics.apply_force(scene, entity.value(), 3.0));
    auto failure = physics.step(scene, 0.0, 0.0);
    EXPECT(!failure);
    EXPECT(failure.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(std::abs(scene.try_get_component<Transform1D>(entity.value())->position - 12.0) < 1e-12);
    auto state = physics.body_snapshot(scene, entity.value());
    EXPECT(state);
    EXPECT(std::abs(state.value().position - 12.0) < 1e-12);
    EXPECT(std::abs(state.value().accumulated_force - 3.0) < 1e-12);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_and_adapter_moves() {
    auto sr = Storm::Scene::create("Moves");
    EXPECT(sr);
    auto source = std::move(sr).value();
    auto entity = source.create_entity("Player");
    EXPECT(entity);
    EXPECT(source.add_component<Transform1D>(entity.value(), Transform1D{4.0}));
    auto pr = PhysicsScene1D::create(source);
    EXPECT(pr);
    auto physics = std::move(pr).value();
    Storm::Scene destination(std::move(source));
    PhysicsScene1D moved(std::move(physics));
    EXPECT(destination.is_alive(entity.value()));
    EXPECT(moved.attach(destination, entity.value()));
    EXPECT(!physics.attach(destination, entity.value()));
    return std::nullopt;
}

struct TestCase { const char* name; std::optional<std::string> (*run)(); };

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"attach_requires_valid_transform", &test_attach_requires_valid_transform},
        TestCase{"scene_position_is_authoritative", &test_scene_position_is_authoritative},
        TestCase{"fixed_step_syncs_transform", &test_fixed_step_syncs_transform},
        TestCase{"teleport_force_and_impulse", &test_teleport_force_and_impulse},
        TestCase{"lookup_and_detach", &test_lookup_and_detach},
        TestCase{"removed_entities_and_components_are_cleaned", &test_removed_entities_and_components_are_cleaned},
        TestCase{"foreign_scene_and_stale_handle_rejected", &test_foreign_scene_and_stale_handle_rejected},
        TestCase{"failed_step_preserves_scene_and_body", &test_failed_step_preserves_scene_and_body},
        TestCase{"scene_and_adapter_moves", &test_scene_and_adapter_moves},
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
