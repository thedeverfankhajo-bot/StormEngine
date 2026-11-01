#include "storm/physics/one_d_scene.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Physics::OneD {

PhysicsScene1D::PhysicsScene1D(SceneId scene_id, PhysicsWorld1D world) noexcept
    : scene_id_(scene_id), world_(std::move(world)) {}

PhysicsScene1D::PhysicsScene1D(PhysicsScene1D&& other) noexcept
    : scene_id_(std::exchange(other.scene_id_, 0U)),
      world_(std::move(other.world_)),
      entries_(std::move(other.entries_)) {}

Result<PhysicsScene1D> PhysicsScene1D::create(
    Scene& scene, PhysicsWorld1DConfig config) {
    if (scene.id() == 0U) {
        return Result<PhysicsScene1D>::failure(Error::make(
            ErrorCode::InvalidState, "Cannot bind PhysicsScene1D to a moved-from Scene"));
    }
    auto world = PhysicsWorld1D::create(config);
    if (!world) return Result<PhysicsScene1D>::failure(world.error());
    return Result<PhysicsScene1D>::success(
        PhysicsScene1D{scene.id(), std::move(world).value()});
}

Result<void> PhysicsScene1D::validate_scene(const Scene& scene) const {
    if (scene_id_ == 0U || world_.world_id() == 0U || scene.id() != scene_id_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "PhysicsScene1D is moved-from or bound to another Scene"));
    }
    return Result<void>::success();
}

PhysicsScene1D::Entry* PhysicsScene1D::find_entity(EntityHandle entity) noexcept {
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [entity](const Entry& e) { return e.entity == entity; });
    return it == entries_.end() ? nullptr : &*it;
}

const PhysicsScene1D::Entry* PhysicsScene1D::find_entity(EntityHandle entity) const noexcept {
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [entity](const Entry& e) { return e.entity == entity; });
    return it == entries_.end() ? nullptr : &*it;
}

PhysicsScene1D::Entry* PhysicsScene1D::find_body(BodyHandle1D body) noexcept {
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [body](const Entry& e) { return e.body == body; });
    return it == entries_.end() ? nullptr : &*it;
}

const PhysicsScene1D::Entry* PhysicsScene1D::find_body(BodyHandle1D body) const noexcept {
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [body](const Entry& e) { return e.body == body; });
    return it == entries_.end() ? nullptr : &*it;
}

Result<BodyHandle1D> PhysicsScene1D::attach(
    Scene& scene, EntityHandle entity, const BodyDefinition1D& definition) {
    auto valid = validate_scene(scene);
    if (!valid) return Result<BodyHandle1D>::failure(valid.error());
    if (!scene.is_alive(entity)) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::InvalidState, "Cannot attach physics to a stale or foreign entity"));
    }

    Math::Transform1D* transform = scene.try_get_component<Math::Transform1D>(entity);
    if (transform == nullptr) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::InvalidState, "A 1D physics body requires Transform1D"));
    }
    if (!transform->is_valid()) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::InvalidArgument, "Transform1D position must be finite"));
    }
    if (find_entity(entity) != nullptr) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::InvalidState, "Entity already has a 1D physics body"));
    }
    if (entries_.size() == entries_.max_size()) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::ResourceExhausted, "PhysicsScene1D mapping capacity is exhausted"));
    }

    try {
        entries_.reserve(entries_.size() + 1U);
    } catch (const std::bad_alloc&) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::ResourceExhausted, "Unable to allocate a physics mapping"));
    } catch (const std::length_error&) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::ResourceExhausted, "Physics mapping exceeds container limits"));
    }

    BodyDefinition1D initialized = definition;
    initialized.position = transform->position;
    auto created = world_.create_body(initialized);
    if (!created) return Result<BodyHandle1D>::failure(created.error());

    // Capacity was reserved first; Entry contains only trivially movable handles.
    entries_.push_back(Entry{entity, created.value()});
    return Result<BodyHandle1D>::success(created.value());
}

Result<void> PhysicsScene1D::detach(Scene& scene, EntityHandle entity) {
    auto valid = validate_scene(scene);
    if (!valid) return valid;
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [entity](const Entry& e) { return e.entity == entity; });
    if (it == entries_.end()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Entity has no attached 1D physics body"));
    }
    auto destroyed = world_.destroy_body(it->body);
    if (!destroyed) return destroyed;
    entries_.erase(it);
    return Result<void>::success();
}

Result<void> PhysicsScene1D::set_position(
    Scene& scene, EntityHandle entity, double position) {
    auto valid = validate_scene(scene);
    if (!valid) return valid;
    auto* transform = scene.is_alive(entity)
        ? scene.try_get_component<Math::Transform1D>(entity) : nullptr;
    Entry* entry = find_entity(entity);
    if (transform == nullptr || entry == nullptr) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Entity needs Transform1D and an attached 1D body"));
    }
    auto moved = world_.set_position(entry->body, position);
    if (!moved) return moved;
    transform->position = position;
    return Result<void>::success();
}

Result<void> PhysicsScene1D::set_velocity(
    Scene& scene, EntityHandle entity, double velocity) {
    auto valid = validate_scene(scene);
    if (!valid) return valid;
    const Entry* entry = find_entity(entity);
    if (!scene.is_alive(entity) ||
        scene.try_get_component<Math::Transform1D>(entity) == nullptr || entry == nullptr) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Entity has no live 1D physics body and Transform1D"));
    }
    return world_.set_velocity(entry->body, velocity);
}

Result<void> PhysicsScene1D::apply_force(
    Scene& scene, EntityHandle entity, double force) {
    auto valid = validate_scene(scene);
    if (!valid) return valid;
    const Entry* entry = find_entity(entity);
    if (!scene.is_alive(entity) ||
        scene.try_get_component<Math::Transform1D>(entity) == nullptr || entry == nullptr) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Entity has no live 1D physics body and Transform1D"));
    }
    return world_.apply_force(entry->body, force);
}

Result<void> PhysicsScene1D::apply_impulse(
    Scene& scene, EntityHandle entity, double impulse) {
    auto valid = validate_scene(scene);
    if (!valid) return valid;
    const Entry* entry = find_entity(entity);
    if (!scene.is_alive(entity) ||
        scene.try_get_component<Math::Transform1D>(entity) == nullptr || entry == nullptr) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Entity has no live 1D physics body and Transform1D"));
    }
    return world_.apply_impulse(entry->body, impulse);
}

Result<BodySnapshot1D> PhysicsScene1D::body_snapshot(
    const Scene& scene, EntityHandle entity) const {
    auto valid = validate_scene(scene);
    if (!valid) return Result<BodySnapshot1D>::failure(valid.error());
    if (!scene.is_alive(entity) ||
        scene.try_get_component<Math::Transform1D>(entity) == nullptr) {
        return Result<BodySnapshot1D>::failure(Error::make(
            ErrorCode::InvalidState, "Entity is stale or no longer has Transform1D"));
    }
    const Entry* entry = find_entity(entity);
    if (entry == nullptr) {
        return Result<BodySnapshot1D>::failure(Error::make(
            ErrorCode::InvalidState, "Entity has no attached 1D physics body"));
    }
    return world_.body_snapshot(entry->body);
}

Result<std::optional<BodyHandle1D>> PhysicsScene1D::body_for_entity(
    const Scene& scene, EntityHandle entity) const {
    auto valid = validate_scene(scene);
    if (!valid) {
        return Result<std::optional<BodyHandle1D>>::failure(valid.error());
    }
    if (!scene.is_alive(entity)) {
        return Result<std::optional<BodyHandle1D>>::failure(Error::make(
            ErrorCode::InvalidState, "Cannot look up a stale or foreign entity"));
    }
    if (scene.try_get_component<Math::Transform1D>(entity) == nullptr) {
        return Result<std::optional<BodyHandle1D>>::success(std::nullopt);
    }
    const Entry* entry = find_entity(entity);
    return Result<std::optional<BodyHandle1D>>::success(
        entry == nullptr ? std::nullopt : std::optional<BodyHandle1D>{entry->body});
}

Result<std::optional<EntityHandle>> PhysicsScene1D::entity_for_body(
    const Scene& scene, BodyHandle1D body) const {
    auto valid = validate_scene(scene);
    if (!valid) {
        return Result<std::optional<EntityHandle>>::failure(valid.error());
    }
    const Entry* entry = find_body(body);
    if (entry == nullptr || !scene.is_alive(entry->entity) ||
        scene.try_get_component<Math::Transform1D>(entry->entity) == nullptr) {
        return Result<std::optional<EntityHandle>>::success(std::nullopt);
    }
    return Result<std::optional<EntityHandle>>::success(entry->entity);
}

Result<StepResult1D> PhysicsScene1D::step(
    Scene& scene, double delta_seconds, double gravity) {
    auto valid = validate_scene(scene);
    if (!valid) return Result<StepResult1D>::failure(valid.error());

    // Polling cannot observe a remove-and-readd that occurs between steps;
    // callers must detach explicitly before replacing Transform1D.
    std::size_t index = 0U;
    while (index < entries_.size()) {
        const Entry& entry = entries_[index];
        if (scene.is_alive(entry.entity) &&
            scene.try_get_component<Math::Transform1D>(entry.entity) != nullptr) {
            ++index;
            continue;
        }
        auto destroyed = world_.destroy_body(entry.body);
        if (!destroyed) return Result<StepResult1D>::failure(destroyed.error());
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
    }

    auto stepped = world_.step(delta_seconds, gravity);
    if (!stepped) return Result<StepResult1D>::failure(stepped.error());

    // Scene structure must not change concurrently with this single-thread-affine call.
    for (const Entry& entry : entries_) {
        auto snapshot = world_.body_snapshot(entry.body);
        if (!snapshot) return Result<StepResult1D>::failure(snapshot.error());
        auto* transform = scene.try_get_component<Math::Transform1D>(entry.entity);
        if (transform == nullptr) {
            return Result<StepResult1D>::failure(Error::make(
                ErrorCode::InvalidState, "Transform1D disappeared during physics synchronization"));
        }
        transform->position = snapshot.value().position;
    }
    return Result<StepResult1D>::success(std::move(stepped).value());
}

std::size_t PhysicsScene1D::body_count() const noexcept {
    return world_.body_count();
}

} // namespace Storm::Physics::OneD
