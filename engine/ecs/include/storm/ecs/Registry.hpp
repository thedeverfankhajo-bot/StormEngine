#pragma once

#include "ComponentStorage.hpp"
#include "ComponentType.hpp"
#include "Entity.hpp"
#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace storm::ecs {

class Registry {
public:
    Entity create() {
        if (!freeIds_.empty()) {
            const Entity::Id id = freeIds_.back();
            freeIds_.pop_back();
            alive_[id] = true;
            return Entity{id, generations_[id]};
        }
        if (alive_.size() >= static_cast<std::size_t>(Entity::kInvalidId))
            throw std::overflow_error("storm::ecs::Registry entity id space exhausted");
        const Entity::Id id = static_cast<Entity::Id>(alive_.size());
        alive_.push_back(true);
        generations_.push_back(0);
        return Entity{id, 0};
    }

    void destroy(Entity entity) {
        if (!valid(entity)) return;
        removeAllComponents(entity.id());
        alive_[entity.id()] = false;
        if (generations_[entity.id()] != Entity::Generation::max()) {
            ++generations_[entity.id()];
            freeIds_.push_back(entity.id());
        }
    }

    bool valid(Entity entity) const noexcept {
        return entity.valid() && entity.id() < alive_.size() &&
               alive_[entity.id()] && generations_[entity.id()] == entity.generation();
    }

    template <typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        if (!valid(entity)) throw std::invalid_argument("storm::ecs::Registry::emplace: invalid entity");
        return storageFor<T>().emplace(entity.id(), std::forward<Args>(args)...);
    }

    template <typename T>
    bool has(Entity entity) const {
        if (!valid(entity)) return false;
        const auto it = storages_.find(componentType<T>());
        return it != storages_.end() && static_cast<const Storage<T>*>(it->second.get())->has(entity.id());
    }

    template <typename T>
    T* tryGet(Entity entity) {
        if (!valid(entity)) return nullptr;
        const auto it = storages_.find(componentType<T>());
        if (it == storages_.end()) return nullptr;
        return static_cast<Storage<T>*>(it->second.get())->tryGet(entity.id());
    }

    template <typename T>
    const T* tryGet(Entity entity) const {
        if (!valid(entity)) return nullptr;
        const auto it = storages_.find(componentType<T>());
        if (it == storages_.end()) return nullptr;
        return static_cast<const Storage<T>*>(it->second.get())->tryGet(entity.id());
    }

    template <typename T>
    T& get(Entity entity) {
        T* value = tryGet<T>(entity);
        assert(value);
        return *value;
    }

    template <typename T>
    const T& get(Entity entity) const {
        const T* value = tryGet<T>(entity);
        assert(value);
        return *value;
    }

    template <typename T>
    void remove(Entity entity) {
        if (!valid(entity)) return;
        const auto it = storages_.find(componentType<T>());
        if (it != storages_.end()) static_cast<Storage<T>*>(it->second.get())->remove(entity.id());
    }

    template <typename... Components, typename Func>
    void each(Func&& func) {
        static_assert(sizeof...(Components) > 0, "Registry::each requires at least one component");
        eachImpl<Components...>(std::forward<Func>(func));
    }

    template <typename T>
    std::size_t componentCount() const {
        const auto it = storages_.find(componentType<T>());
        return it == storages_.end() ? 0 : static_cast<const Storage<T>*>(it->second.get())->size();
    }

private:
    struct IStorage {
        virtual ~IStorage() = default;
        virtual void remove(Entity::Id id) = 0;
    };

    template <typename T>
    struct Storage final : IStorage {
        ComponentStorage<T> data;
        template <typename... Args>
        T& emplace(Entity::Id id, Args&&... args) { return data.emplace(id, std::forward<Args>(args)...); }
        bool has(Entity::Id id) const noexcept { return data.has(id); }
        T* tryGet(Entity::Id id) noexcept { return data.tryGet(id); }
        const T* tryGet(Entity::Id id) const noexcept { return data.tryGet(id); }
        void remove(Entity::Id id) override { data.remove(id); }
        std::size_t size() const noexcept { return data.size(); }
        Entity::Id entityAt(std::size_t index) const noexcept { return data.entityAt(index); }
    };

    template <typename T>
    Storage<T>& storageFor() {
        const ComponentType type = componentType<T>();
        auto it = storages_.find(type);
        if (it == storages_.end()) {
            auto storage = std::make_unique<Storage<T>>();
            auto* raw = storage.get();
            storages_.emplace(type, std::move(storage));
            return *raw;
        }
        return *static_cast<Storage<T>*>(it->second.get());
    }

    template <typename First, typename... Rest, typename Func>
    void eachImpl(Func&& func) {
        const Storage<First>* storage = findStorage<First>();
        if (!storage) return;
        for (std::size_t i = 0; i < storage->size(); ++i) {
            const Entity::Id id = storage->entityAt(i);
            if (id >= alive_.size() || !alive_[id]) continue;
            Entity entity{id, generations_[id]};
            if ((has<Rest>(entity) && ...))
                std::invoke(std::forward<Func>(func), entity, get<First>(entity), get<Rest>(entity)...);
        }
    }

    template <typename T>
    const Storage<T>* findStorage() const noexcept {
        const auto it = storages_.find(componentType<T>());
        return it == storages_.end() ? nullptr : static_cast<const Storage<T>*>(it->second.get());
    }

    void removeAllComponents(Entity::Id id) {
        for (auto& [type, storage] : storages_) {
            (void)type;
            storage->remove(id);
        }
    }

    std::vector<bool> alive_;
    std::vector<Entity::Generation> generations_;
    std::vector<Entity::Id> freeIds_;
    std::unordered_map<ComponentType, std::unique_ptr<IStorage>> storages_;
};

} // namespace storm::ecs
