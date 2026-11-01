#pragma once

#include "storm/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Storm {

using SceneId = std::uint64_t;
using EntityIndex = std::uint32_t;
using EntityGeneration = std::uint32_t;

struct EntityHandle final {
    SceneId scene_id{0U};
    EntityIndex index{0U};
    EntityGeneration generation{0U};

    [[nodiscard]] friend bool operator==(
        const EntityHandle& lhs,
        const EntityHandle& rhs) noexcept = default;
};

// Owns entity identity, hierarchy and type-safe, scene-local component storage.
// Handles remain valid across Scene moves, but not across entity destruction.
class Scene final {
public:
    [[nodiscard]] static Result<Scene> create(std::string name);

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&& other) noexcept;
    Scene& operator=(Scene&& other) noexcept;
    ~Scene() = default;

    [[nodiscard]] SceneId id() const noexcept;
    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] std::size_t entity_count() const noexcept;

    [[nodiscard]] Result<EntityHandle> create_entity(std::string name = {});
    [[nodiscard]] Result<void> destroy_entity(EntityHandle entity);
    [[nodiscard]] bool is_alive(EntityHandle entity) const noexcept;
    [[nodiscard]] Result<std::string_view> entity_name(EntityHandle entity) const;
    [[nodiscard]] Result<void> rename_entity(EntityHandle entity, std::string name);

    // A null parent detaches the entity. Reparenting rejects self-links and cycles.
    [[nodiscard]] Result<void> set_parent(
        EntityHandle child,
        std::optional<EntityHandle> parent);
    [[nodiscard]] Result<std::optional<EntityHandle>> parent_of(
        EntityHandle entity) const;
    // The returned view is invalidated by hierarchy changes or Scene destruction.
    [[nodiscard]] Result<std::span<const EntityHandle>> children_of(
        EntityHandle entity) const;

    template <typename Component, typename... Args>
    [[nodiscard]] Result<std::reference_wrapper<Component>> add_component(
        EntityHandle entity,
        Args&&... args) {
        static_assert(
            std::is_object_v<Component> &&
                !std::is_const_v<Component> &&
                !std::is_volatile_v<Component>,
            "Components must be non-cv-qualified object types");
        static_assert(
            std::is_nothrow_destructible_v<Component>,
            "Component destructors must be noexcept");

        if (!is_alive(entity)) {
            return Result<std::reference_wrapper<Component>>::failure(
                invalid_entity_error());
        }

        ComponentStorage<Component>& storage = get_or_create_storage<Component>();
        const auto [iterator, inserted] = storage.values.try_emplace(
            entity.index,
            std::forward<Args>(args)...);
        if (!inserted) {
            return Result<std::reference_wrapper<Component>>::failure(Error::make(
                ErrorCode::InvalidState,
                "The entity already owns a component of this type"));
        }

        return Result<std::reference_wrapper<Component>>::success(
            std::ref(iterator->second));
    }

    template <typename Component>
    [[nodiscard]] Component* try_get_component(EntityHandle entity) noexcept {
        static_assert(
            std::is_object_v<Component> &&
                !std::is_const_v<Component> &&
                !std::is_volatile_v<Component>,
            "Components must be non-cv-qualified object types");

        if (!is_alive(entity)) {
            return nullptr;
        }

        ComponentStorage<Component>* storage = find_storage<Component>();
        if (storage == nullptr) {
            return nullptr;
        }

        const auto iterator = storage->values.find(entity.index);
        return iterator == storage->values.end() ? nullptr : &iterator->second;
    }

    template <typename Component>
    [[nodiscard]] const Component* try_get_component(EntityHandle entity) const noexcept {
        static_assert(
            std::is_object_v<Component> &&
                !std::is_const_v<Component> &&
                !std::is_volatile_v<Component>,
            "Components must be non-cv-qualified object types");

        if (!is_alive(entity)) {
            return nullptr;
        }

        const ComponentStorage<Component>* storage = find_storage<Component>();
        if (storage == nullptr) {
            return nullptr;
        }

        const auto iterator = storage->values.find(entity.index);
        return iterator == storage->values.end() ? nullptr : &iterator->second;
    }

    template <typename Component>
    [[nodiscard]] Result<void> remove_component(EntityHandle entity) {
        static_assert(
            std::is_object_v<Component> &&
                !std::is_const_v<Component> &&
                !std::is_volatile_v<Component>,
            "Components must be non-cv-qualified object types");

        if (!is_alive(entity)) {
            return Result<void>::failure(invalid_entity_error());
        }

        ComponentStorage<Component>* storage = find_storage<Component>();
        if (storage == nullptr || storage->values.erase(entity.index) == 0U) {
            return Result<void>::failure(Error::make(
                ErrorCode::InvalidState,
                "The entity does not own a component of this type"));
        }

        return Result<void>::success();
    }

private:
    struct EntitySlot final {
        EntityGeneration generation{1U};
        bool alive{false};
        std::string name;
        std::optional<EntityHandle> parent;
        std::vector<EntityHandle> children;
    };

    struct ComponentStorageBase {
        virtual ~ComponentStorageBase() = default;
        virtual void erase(EntityIndex entity) noexcept = 0;
    };

    template <typename Component>
    struct ComponentStorage final : ComponentStorageBase {
        std::unordered_map<EntityIndex, Component> values;

        void erase(EntityIndex entity) noexcept override {
            static_cast<void>(values.erase(entity));
        }
    };

    explicit Scene(SceneId scene_id, std::string name) noexcept;

    [[nodiscard]] Error invalid_entity_error() const;
    [[nodiscard]] bool owns(EntityHandle entity) const noexcept;

    template <typename Component>
    [[nodiscard]] ComponentStorage<Component>* find_storage() noexcept {
        const auto iterator = component_storages_.find(std::type_index(typeid(Component)));
        if (iterator == component_storages_.end()) {
            return nullptr;
        }
        return static_cast<ComponentStorage<Component>*>(iterator->second.get());
    }

    template <typename Component>
    [[nodiscard]] const ComponentStorage<Component>* find_storage() const noexcept {
        const auto iterator = component_storages_.find(std::type_index(typeid(Component)));
        if (iterator == component_storages_.end()) {
            return nullptr;
        }
        return static_cast<const ComponentStorage<Component>*>(iterator->second.get());
    }

    template <typename Component>
    [[nodiscard]] ComponentStorage<Component>& get_or_create_storage() {
        const std::type_index key(typeid(Component));
        const auto iterator = component_storages_.find(key);
        if (iterator != component_storages_.end()) {
            return *static_cast<ComponentStorage<Component>*>(iterator->second.get());
        }

        auto storage = std::make_unique<ComponentStorage<Component>>();
        ComponentStorage<Component>* storage_pointer = storage.get();
        component_storages_.emplace(key, std::move(storage));
        return *storage_pointer;
    }

    SceneId scene_id_{0U};
    std::string name_;
    std::vector<EntitySlot> entities_;
    std::vector<EntityIndex> free_entity_indices_;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>>
        component_storages_;
};

} // namespace Storm
