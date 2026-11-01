#include "storm/core/scene.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <new>
#include <stdexcept>
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
        EntitySlot& slot = entities_[index];
        slot.name = std::move(name);
        slot.parent.reset();
        slot.children.clear();
        free_entity_indices_.pop_back();
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
    try {
        entities_.push_back(EntitySlot{
            .generation = 1U,
            .alive = true,
            .name = std::move(name),
            .parent = std::nullopt,
            .children = {},
        });
    } catch (const std::bad_alloc&) {
        return Result<EntityHandle>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "The Scene could not allocate another entity slot"));
    }

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

    // Reserve the free-list entry before mutating hierarchy or components.
    if (can_reuse) {
        try {
            free_entity_indices_.push_back(entity.index);
        } catch (const std::bad_alloc&) {
            return Result<void>::failure(Error::make(
                ErrorCode::ResourceExhausted,
                "The Scene could not reserve an entity slot for reuse"));
        }
    }

    // Unlink this entity from its parent before invalidating the handle.
    if (slot.parent.has_value() && owns(*slot.parent)) {
        std::vector<EntityHandle>& siblings =
            entities_[slot.parent->index].children;
        const auto child_iterator = std::find(siblings.begin(), siblings.end(), entity);
        if (child_iterator != siblings.end()) {
            siblings.erase(child_iterator);
        }
    }

    // Deleting a parent orphans its direct children; their own subtrees remain intact.
    for (const EntityHandle child : slot.children) {
        if (owns(child)) {
            EntitySlot& child_slot = entities_[child.index];
            if (child_slot.parent == entity) {
                child_slot.parent.reset();
            }
        }
    }
    slot.parent.reset();
    slot.children.clear();

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

Result<void> Scene::set_parent(
    EntityHandle child,
    std::optional<EntityHandle> parent) {
    if (!owns(child)) {
        return Result<void>::failure(invalid_entity_error());
    }
    if (parent.has_value() && !owns(*parent)) {
        return Result<void>::failure(invalid_entity_error());
    }
    if (parent.has_value() && *parent == child) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "An entity cannot be its own parent"));
    }

    EntitySlot& child_slot = entities_[child.index];
    if (child_slot.parent == parent) {
        return Result<void>::success();
    }

    if (parent.has_value()) {
        // Walk the proposed parent's ancestry before changing either side.
        std::optional<EntityHandle> ancestor = parent;
        std::size_t visited = 0U;
        while (ancestor.has_value()) {
            if (*ancestor == child) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument,
                    "Reparenting would create a hierarchy cycle"));
            }
            if (!owns(*ancestor)) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidState,
                    "The existing hierarchy contains a stale parent handle"));
            }
            if (++visited > entities_.size()) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidState,
                    "The existing hierarchy contains a cycle"));
            }
            ancestor = entities_[ancestor->index].parent;
        }

        std::vector<EntityHandle>& new_children =
            entities_[parent->index].children;
        const auto existing = std::find(
            new_children.begin(), new_children.end(), child);
        if (existing == new_children.end()) {
            try {
                new_children.push_back(child);
            } catch (const std::bad_alloc&) {
                return Result<void>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "The new parent could not reserve a child entry"));
            } catch (const std::length_error&) {
                return Result<void>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "The new parent has reached its child capacity"));
            }
        }
    }

    // All potentially allocating work is complete. The old edge can now be removed.
    if (child_slot.parent.has_value() && owns(*child_slot.parent)) {
        std::vector<EntityHandle>& old_children =
            entities_[child_slot.parent->index].children;
        const auto old_entry = std::find(
            old_children.begin(), old_children.end(), child);
        if (old_entry != old_children.end()) {
            old_children.erase(old_entry);
        }
    }

    child_slot.parent = parent;
    return Result<void>::success();
}

Result<std::optional<EntityHandle>> Scene::parent_of(EntityHandle entity) const {
    if (!owns(entity)) {
        return Result<std::optional<EntityHandle>>::failure(invalid_entity_error());
    }
    return Result<std::optional<EntityHandle>>::success(entities_[entity.index].parent);
}

Result<std::span<const EntityHandle>> Scene::children_of(EntityHandle entity) const {
    if (!owns(entity)) {
        return Result<std::span<const EntityHandle>>::failure(invalid_entity_error());
    }
    const std::vector<EntityHandle>& children = entities_[entity.index].children;
    return Result<std::span<const EntityHandle>>::success(
        std::span<const EntityHandle>{children.data(), children.size()});
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
