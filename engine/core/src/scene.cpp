#include "storm/core/scene.hpp"

#include <atomic>
#include <limits>
#include <string>
#include <utility>

namespace Storm {
namespace {

std::atomic<SceneId> next_scene_id{1U};

[[nodiscard]] Result<SceneId> allocate_scene_id() {
    SceneId candidate = next_scene_id.load(std::memory_order_relaxed);
    for (;;) {
        if (candidate == 0U || candidate == std::numeric_limits<SceneId>::max()) {
            return Result<SceneId>::failure(Error::make(
                ErrorCode::ResourceExhausted,
                "The process-wide Scene identifier space is exhausted"));
        }

        if (next_scene_id.compare_exchange_weak(
                candidate,
                candidate + 1U,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return Result<SceneId>::success(candidate);
        }
    }
}

[[nodiscard]] bool contains_non_whitespace(std::string_view value) noexcept {
    return value.find_first_not_of(" \t\n\r\f\v") != std::string_view::npos;
}

} // namespace

Scene::Scene(SceneId scene_id, std::string name) noexcept
    : scene_id_(scene_id), name_(std::move(name)) {}

Scene::Scene(Scene&& other) noexcept
    : scene_id_(std::exchange(other.scene_id_, 0U)),
      name_(std::move(other.name_)),
      entities_(std::move(other.entities_)),
      free_entity_indices_(std::move(other.free_entity_indices_)),
      component_storages_(std::move(other.component_storages_)) {}

Scene& Scene::operator=(Scene&& other) noexcept {
    if (this != &other) {
        scene_id_ = std::exchange(other.scene_id_, 0U);
        name_ = std::move(other.name_);
        entities_ = std::move(other.entities_);
        free_entity_indices_ = std::move(other.free_entity_indices_);
        component_storages_ = std::move(other.component_storages_);
    }
    return *this;
}

Result<Scene> Scene::create(std::string name) {
    if (!contains_non_whitespace(name)) {
        return Result<Scene>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A Scene name must contain at least one non-whitespace character"));
    }

    Result<SceneId> scene_id = allocate_scene_id();
    if (!scene_id) {
        return Result<Scene>::failure(scene_id.error());
    }

    return Result<Scene>::success(
        Scene{scene_id.value(), std::move(name)});
}

SceneId Scene::id() const noexcept {
    return scene_id_;
}

std::string_view Scene::name() const noexcept {
    return name_;
}

std::size_t Scene::entity_count() const noexcept {
    std::size_t count = 0U;
    for (const EntitySlot& entity : entities_) {
        if (entity.alive) {
            ++count;
        }
    }
    return count;
}

Result<EntityHandle> Scene::create_entity(std::string name) {
    if (scene_id_ == 0U) {
        return Result<EntityHandle>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from Scene cannot create entities"));
    }

    EntityIndex index = 0U;
    if (!free_entity_indices_.empty()) {
        index = free_entity_indices_.back();
        free_entity_indices_.pop_back();

        EntitySlot& slot = entities_[index];
        slot.name = std::move(name);
        slot.alive = true;
        return Result<EntityHandle>::success(EntityHandle{
            .scene_id = scene_id_,
            .index = index,
            .generation = slot.generation,
        });
    }

    if (entities_.size() >=
        static_cast<std::size_t>(std::numeric_limits<EntityIndex>::max())) {
        return Result<EntityHandle>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "The Scene entity index space is exhausted"));
    }

    index = static_cast<EntityIndex>(entities_.size());
    entities_.push_back(EntitySlot{
        .generation = 1U,
        .alive = true,
        .name = std::move(name),
    });

    return Result<EntityHandle>::success(EntityHandle{
        .scene_id = scene_id_,
        .index = index,
        .generation = 1U,
    });
}

Result<void> Scene::destroy_entity(EntityHandle entity) {
    if (!is_alive(entity)) {
        return Result<void>::failure(invalid_entity_error());
    }

    EntitySlot& slot = entities_[entity.index];
    const bool can_reuse = slot.generation !=
        std::numeric_limits<EntityGeneration>::max();

    // Allocate free-list capacity before mutating the live entity. If the
    // allocation fails, the entity remains fully usable.
    if (can_reuse) {
        free_entity_indices_.push_back(entity.index);
    }

    for (auto& [component_type, storage] : component_storages_) {
        static_cast<void>(component_type);
        storage->erase(entity.index);
    }

    slot.alive = false;
    slot.name.clear();
    if (can_reuse) {
        ++slot.generation;
    }

    return Result<void>::success();
}

bool Scene::is_alive(EntityHandle entity) const noexcept {
    return owns(entity);
}

Result<std::string_view> Scene::entity_name(EntityHandle entity) const {
    if (!is_alive(entity)) {
        return Result<std::string_view>::failure(invalid_entity_error());
    }
    return Result<std::string_view>::success(entities_[entity.index].name);
}

Result<void> Scene::rename_entity(EntityHandle entity, std::string name) {
    if (!is_alive(entity)) {
        return Result<void>::failure(invalid_entity_error());
    }
    entities_[entity.index].name = std::move(name);
    return Result<void>::success();
}

Error Scene::invalid_entity_error() const {
    return Error::make(
        ErrorCode::InvalidState,
        "The EntityHandle is stale, invalid, or belongs to another Scene");
}

bool Scene::owns(EntityHandle entity) const noexcept {
    if (scene_id_ == 0U || entity.scene_id != scene_id_ ||
        entity.index >= entities_.size()) {
        return false;
    }

    const EntitySlot& slot = entities_[entity.index];
    return slot.alive && slot.generation == entity.generation;
}

} // namespace Storm
