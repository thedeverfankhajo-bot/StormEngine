#pragma once

#include "storm/core/stable_id.hpp"
#include "storm/core/stable_id_generator.hpp"
#include "storm/core/status.hpp"
#include "storm/scene/dimension.hpp"
#include "storm/scene/transform.hpp"

#include <cstddef>
#include <map>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace storm::scene {

class Scene;

class Entity final {
public:
    static constexpr std::size_t max_name_bytes = 128U;

    [[nodiscard]] const core::StableId& id() const noexcept {
        return id_;
    }

    [[nodiscard]] std::string_view name() const noexcept {
        return name_;
    }

    [[nodiscard]] const Transform& transform() const noexcept {
        return transform_;
    }

    // Rejects non-finite values and transforms from a different scene dimension.
    // The existing transform is left untouched if validation fails.
    [[nodiscard]] core::Status set_transform(const Transform& transform) {
        if (!transform.is_finite()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "entity transform contains a non-finite value");
        }
        if (transform.dimension() != transform_.dimension()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "entity transform dimension does not match");
        }

        transform_ = transform;
        return core::Status::success();
    }

private:
    friend class Scene;

    Entity(core::StableId id, std::string name, Transform transform)
        : id_(id), name_(std::move(name)), transform_(std::move(transform)) {}

    core::StableId id_{};
    std::string name_{};
    Transform transform_{};
};

// Owns its entities and their names/transforms. Entity IDs are OS-random UUIDv4 values.
// This first in-memory scene model is single-owner-thread only and does not serialize data.
class Scene final {
public:
    static constexpr std::size_t default_max_entities = 10'000U;
    static constexpr std::size_t maximum_allowed_entities = 1'000'000U;

    explicit Scene(Dimension dimension,
                   std::size_t max_entities = default_max_entities) noexcept
        : dimension_(dimension), max_entities_(max_entities) {}

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    [[nodiscard]] core::Status validate() const {
        if (!is_valid_dimension(dimension_)) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "scene dimension is invalid");
        }
        if (max_entities_ == 0U || max_entities_ > maximum_allowed_entities) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "scene entity capacity is out of range");
        }
        return core::Status::success();
    }

    [[nodiscard]] Dimension dimension() const noexcept {
        return dimension_;
    }

    [[nodiscard]] std::size_t entity_count() const noexcept {
        return entities_.size();
    }

    [[nodiscard]] std::size_t max_entities() const noexcept {
        return max_entities_;
    }

    // The output ID changes only after entity insertion succeeds.
    [[nodiscard]] core::Status create_entity(std::string_view name, core::StableId& output) {
        const core::Status configuration = validate();
        if (!configuration.ok()) {
            return configuration;
        }
        if (name.empty() || name.size() > Entity::max_name_bytes ||
            name.find('\0') != std::string_view::npos) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument,
                "entity name is empty, too long, or contains NUL");
        }
        if (entities_.size() >= max_entities_) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "scene entity capacity has been reached");
        }

        const auto initial_transform = Transform::for_dimension(dimension_);
        if (!initial_transform.has_value()) {
            return core::Status::failure(
                core::ErrorCode::Internal, "validated scene dimension has no transform type");
        }

        core::StableId id;
        const core::Status generated = core::StableIdGenerator::generate_v4(id);
        if (!generated.ok()) {
            return generated;
        }

        try {
            Entity candidate{id, std::string{name.data(), name.size()}, *initial_transform};
            const auto inserted = entities_.emplace(id, std::move(candidate));
            if (!inserted.second) {
                return core::Status::failure(
                    core::ErrorCode::AlreadyExists, "generated entity ID already exists in scene");
            }
        } catch (const std::bad_alloc&) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "scene could not allocate entity storage");
        }

        output = id;
        return core::Status::success();
    }

    [[nodiscard]] core::Status destroy_entity(const core::StableId& id) {
        const core::Status configuration = validate();
        if (!configuration.ok()) {
            return configuration;
        }
        if (!id.is_valid()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "cannot destroy an entity with an invalid ID");
        }

        const auto found = entities_.find(id);
        if (found == entities_.end()) {
            return core::Status::failure(core::ErrorCode::NotFound, "entity was not found");
        }

        entities_.erase(found);
        return core::Status::success();
    }

    // Returned pointers are non-owning: destruction of that entity or the scene invalidates them.
    [[nodiscard]] Entity* find_entity(const core::StableId& id) noexcept {
        const auto found = entities_.find(id);
        return found == entities_.end() ? nullptr : &found->second;
    }

    [[nodiscard]] const Entity* find_entity(const core::StableId& id) const noexcept {
        const auto found = entities_.find(id);
        return found == entities_.end() ? nullptr : &found->second;
    }

private:
    Dimension dimension_;
    std::size_t max_entities_;
    std::map<core::StableId, Entity, core::StableIdLess> entities_{};
};

} // namespace storm::scene
