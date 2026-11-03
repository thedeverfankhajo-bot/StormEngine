#include "storm/physics/two_d_scene.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Physics::TwoD {
namespace {

[[nodiscard]] bool to_scene_position(
    Vector2 position,
    Math::Vector2& converted) noexcept {
    constexpr double maximum_float =
        static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(position.x) ||
        !std::isfinite(position.y) ||
        std::abs(position.x) > maximum_float ||
        std::abs(position.y) > maximum_float) {
        return false;
    }

    const Math::Vector2 candidate{
        static_cast<float>(position.x),
        static_cast<float>(position.y),
    };
    if (!candidate.is_finite()) {
        return false;
    }
    converted = candidate;
    return true;
}

[[nodiscard]] Error invalid_binding_error(const char* message) {
    return Error::make(ErrorCode::InvalidState, message);
}

} // namespace

PhysicsScene2D::PhysicsScene2D(SceneId scene_id, PhysicsWorld2D world) noexcept
    : scene_id_(scene_id), world_(std::move(world)) {}

PhysicsScene2D::PhysicsScene2D(PhysicsScene2D&& other) noexcept
    : scene_id_(std::exchange(other.scene_id_, 0U)),
      world_(std::move(other.world_)),
      entries_(std::move(other.entries_)) {}

Result<PhysicsScene2D> PhysicsScene2D::create(
    Scene& scene,
    PhysicsWorld2DConfig config) {
    if (scene.id() == 0U) {
        return Result<PhysicsScene2D>::failure(Error::make(
            ErrorCode::InvalidState,
            "Cannot bind PhysicsScene2D to a moved-from Scene"));
    }

    // Transform2D stores translation as float while Physics2D uses double.
    // Bound the bridge's coordinate range so every committed position can be
    // represented by its Scene component without producing infinity.
    constexpr double maximum_float =
        static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(config.maximum_absolute_coordinate) ||
        config.maximum_absolute_coordinate <= 0.0 ||
        config.maximum_absolute_coordinate > maximum_float) {
        return Result<PhysicsScene2D>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "PhysicsScene2D coordinate limit must fit finite Transform2D float values"));
    }

    auto world = PhysicsWorld2D::create(config);
    if (!world) {
        return Result<PhysicsScene2D>::failure(world.error());
    }
    return Result<PhysicsScene2D>::success(
        PhysicsScene2D{scene.id(), std::move(world).value()});
}

Result<void> PhysicsScene2D::validate_scene(const Scene& scene) const {
    if (scene_id_ == 0U || world_.world_id() == 0U || scene.id() != scene_id_) {
        return Result<void>::failure(invalid_binding_error(
            "PhysicsScene2D is moved-from or bound to another Scene"));
    }
    return Result<void>::success();
}

Result<void> PhysicsScene2D::validate_entity_transform(
    const Scene& scene,
    EntityHandle entity) const {
    if (!scene.is_alive(entity)) {
        return Result<void>::failure(invalid_binding_error(
            "Physics2D requires a live entity from the bound Scene"));
    }
    const Math::Transform2D* transform =
        scene.try_get_component<Math::Transform2D>(entity);
    if (transform == nullptr) {
        return Result<void>::failure(invalid_binding_error(
            "A 2D physics body requires Transform2D"));
    }
    if (!transform->is_valid()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Transform2D translation, rotation, and scale must be finite"));
    }

    const auto parent = scene.parent_of(entity);
    if (!parent) {
        return Result<void>::failure(parent.error());
    }
    if (parent.value().has_value()) {
        return Result<void>::failure(invalid_binding_error(
            "PhysicsScene2D currently requires an unparented entity"));
    }
    return Result<void>::success();
}

PhysicsScene2D::Entry* PhysicsScene2D::find_entity(EntityHandle entity) noexcept {
    const auto iterator = std::find_if(
        entries_.begin(), entries_.end(),
        [entity](const Entry& entry) { return entry.entity == entity; });
    return iterator == entries_.end() ? nullptr : &*iterator;
}

const PhysicsScene2D::Entry* PhysicsScene2D::find_entity(
    EntityHandle entity) const noexcept {
    const auto iterator = std::find_if(
        entries_.begin(), entries_.end(),
        [entity](const Entry& entry) { return entry.entity == entity; });
    return iterator == entries_.end() ? nullptr : &*iterator;
}

PhysicsScene2D::Entry* PhysicsScene2D::find_body(BodyHandle2D body) noexcept {
    const auto iterator = std::find_if(
        entries_.begin(), entries_.end(),
        [body](const Entry& entry) { return entry.body == body; });
    return iterator == entries_.end() ? nullptr : &*iterator;
}

const PhysicsScene2D::Entry* PhysicsScene2D::find_body(
    BodyHandle2D body) const noexcept {
    const auto iterator = std::find_if(
        entries_.begin(), entries_.end(),
        [body](const Entry& entry) { return entry.body == body; });
    return iterator == entries_.end() ? nullptr : &*iterator;
}

Result<BodyHandle2D> PhysicsScene2D::attach(
    Scene& scene,
    EntityHandle entity,
    const BodyDefinition2D& definition) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return Result<BodyHandle2D>::failure(valid_scene.error());
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return Result<BodyHandle2D>::failure(valid_entity.error());
    }
    if (find_entity(entity) != nullptr) {
        return Result<BodyHandle2D>::failure(invalid_binding_error(
            "Entity already has a 2D physics body"));
    }
    if (entries_.size() == entries_.max_size()) {
        return Result<BodyHandle2D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "PhysicsScene2D mapping capacity is exhausted"));
    }

    try {
        entries_.reserve(entries_.size() + 1U);
    } catch (const std::bad_alloc&) {
        return Result<BodyHandle2D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Unable to allocate a 2D physics mapping"));
    } catch (const std::length_error&) {
        return Result<BodyHandle2D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "PhysicsScene2D mapping exceeds container limits"));
    }

    const Math::Transform2D* transform =
        scene.try_get_component<Math::Transform2D>(entity);
    BodyDefinition2D initialized = definition;
    initialized.position = Vector2{
        static_cast<double>(transform->translation.x),
        static_cast<double>(transform->translation.y),
    };

    auto created = world_.create_body(initialized);
    if (!created) {
        return Result<BodyHandle2D>::failure(created.error());
    }

    entries_.push_back(Entry{entity, created.value()});
    return Result<BodyHandle2D>::success(created.value());
}

Result<void> PhysicsScene2D::detach(Scene& scene, EntityHandle entity) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return valid_scene;
    }

    const auto iterator = std::find_if(
        entries_.begin(), entries_.end(),
        [entity](const Entry& entry) { return entry.entity == entity; });
    if (iterator == entries_.end()) {
        return Result<void>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }

    auto destroyed = world_.destroy_body(iterator->body);
    if (!destroyed) {
        return destroyed;
    }
    entries_.erase(iterator);
    return Result<void>::success();
}

Result<void> PhysicsScene2D::set_position(
    Scene& scene,
    EntityHandle entity,
    Vector2 position) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return valid_scene;
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return valid_entity;
    }

    Math::Vector2 scene_position{};
    if (!to_scene_position(position, scene_position)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Physics2D position cannot be represented by Transform2D"));
    }

    Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<void>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }

    auto moved = world_.set_position(entry->body, position);
    if (!moved) {
        return moved;
    }
    scene.try_get_component<Math::Transform2D>(entity)->translation = scene_position;
    return Result<void>::success();
}

Result<void> PhysicsScene2D::set_velocity(
    Scene& scene,
    EntityHandle entity,
    Vector2 velocity) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return valid_scene;
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return valid_entity;
    }

    const Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<void>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }
    return world_.set_velocity(entry->body, velocity);
}

Result<void> PhysicsScene2D::apply_force(
    Scene& scene,
    EntityHandle entity,
    Vector2 force) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return valid_scene;
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return valid_entity;
    }

    const Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<void>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }
    return world_.apply_force(entry->body, force);
}

Result<void> PhysicsScene2D::apply_impulse(
    Scene& scene,
    EntityHandle entity,
    Vector2 impulse) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return valid_scene;
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return valid_entity;
    }

    const Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<void>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }
    return world_.apply_impulse(entry->body, impulse);
}

Result<BodySnapshot2D> PhysicsScene2D::body_snapshot(
    const Scene& scene,
    EntityHandle entity) const {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return Result<BodySnapshot2D>::failure(valid_scene.error());
    }
    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return Result<BodySnapshot2D>::failure(valid_entity.error());
    }

    const Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<BodySnapshot2D>::failure(invalid_binding_error(
            "Entity has no attached 2D physics body"));
    }
    return world_.body_snapshot(entry->body);
}

Result<std::optional<BodyHandle2D>> PhysicsScene2D::body_for_entity(
    const Scene& scene,
    EntityHandle entity) const {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return Result<std::optional<BodyHandle2D>>::failure(valid_scene.error());
    }
    if (!scene.is_alive(entity)) {
        return Result<std::optional<BodyHandle2D>>::failure(invalid_binding_error(
            "Cannot look up a stale or foreign entity"));
    }
    if (scene.try_get_component<Math::Transform2D>(entity) == nullptr) {
        return Result<std::optional<BodyHandle2D>>::success(std::nullopt);
    }

    const auto valid_entity = validate_entity_transform(scene, entity);
    if (!valid_entity) {
        return Result<std::optional<BodyHandle2D>>::failure(valid_entity.error());
    }
    const Entry* entry = find_entity(entity);
    return Result<std::optional<BodyHandle2D>>::success(
        entry == nullptr ? std::nullopt : std::optional<BodyHandle2D>{entry->body});
}

Result<std::optional<EntityHandle>> PhysicsScene2D::entity_for_body(
    const Scene& scene,
    BodyHandle2D body) const {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return Result<std::optional<EntityHandle>>::failure(valid_scene.error());
    }

    const Entry* entry = find_body(body);
    if (entry == nullptr || !scene.is_alive(entry->entity) ||
        scene.try_get_component<Math::Transform2D>(entry->entity) == nullptr) {
        return Result<std::optional<EntityHandle>>::success(std::nullopt);
    }
    const auto valid_entity = validate_entity_transform(scene, entry->entity);
    if (!valid_entity) {
        return Result<std::optional<EntityHandle>>::failure(valid_entity.error());
    }
    return Result<std::optional<EntityHandle>>::success(entry->entity);
}

Result<StepResult2D> PhysicsScene2D::step(
    Scene& scene,
    double delta_seconds,
    Vector2 gravity) {
    const auto valid_scene = validate_scene(scene);
    if (!valid_scene) {
        return Result<StepResult2D>::failure(valid_scene.error());
    }

    // Reject newly parented entities before the physics world mutates.
    for (const Entry& entry : entries_) {
        if (scene.is_alive(entry.entity) &&
            scene.try_get_component<Math::Transform2D>(entry.entity) != nullptr) {
            const auto valid_entity = validate_entity_transform(scene, entry.entity);
            if (!valid_entity) {
                return Result<StepResult2D>::failure(valid_entity.error());
            }
        }
    }

    // Retire bodies whose entities or Transform2D components were removed.
    // A remove-and-readd between adapter steps must be paired with detach().
    std::size_t index = 0U;
    while (index < entries_.size()) {
        const Entry& entry = entries_[index];
        if (scene.is_alive(entry.entity) &&
            scene.try_get_component<Math::Transform2D>(entry.entity) != nullptr) {
            ++index;
            continue;
        }

        auto destroyed = world_.destroy_body(entry.body);
        if (!destroyed) {
            return Result<StepResult2D>::failure(destroyed.error());
        }
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
    }

    auto stepped = world_.step(delta_seconds, gravity);
    if (!stepped) {
        return Result<StepResult2D>::failure(stepped.error());
    }

    // Scene structure must not change concurrently with this single-thread-affine call.
    for (const Entry& entry : entries_) {
        auto snapshot = world_.body_snapshot(entry.body);
        if (!snapshot) {
            return Result<StepResult2D>::failure(snapshot.error());
        }

        Math::Vector2 scene_position{};
        if (!to_scene_position(snapshot.value().position, scene_position)) {
            return Result<StepResult2D>::failure(Error::make(
                ErrorCode::InvalidState,
                "Physics2D produced a position not representable by Transform2D"));
        }

        auto* transform = scene.try_get_component<Math::Transform2D>(entry.entity);
        if (transform == nullptr) {
            return Result<StepResult2D>::failure(Error::make(
                ErrorCode::InvalidState,
                "Transform2D disappeared during physics synchronization"));
        }
        transform->translation = scene_position;
    }

    return Result<StepResult2D>::success(std::move(stepped).value());
}

std::size_t PhysicsScene2D::body_count() const noexcept {
    return world_.body_count();
}

} // namespace Storm::Physics::TwoD
