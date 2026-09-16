#pragma once

#include "Entity.hpp"
#include "ComponentType.hpp"
#include <cassert>
#include <cstddef>
#include <memory>
#include <typeindex>
#include <unordered_map>
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

        const Entity::Id id = static_cast<Entity::Id>(alive_.size());
        alive_.push_back(true);
        generations_.push_back(0);
        return Entity{id, 0};
    }

    void destroy(Entity entity) {
        if (!valid(entity)) return;
        alive_[entity.id()] = false;
        ++generations_[entity.id()];
        freeIds_.push_back(entity.id());
    }

    bool valid(Entity entity) const noexcept {
        return entity.valid() &&
               entity.id() < alive_.size() &&
               alive_[entity.id()] &&
               generations_[entity.id()] == entity.generation();
    }

    template <typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        assert(valid(entity));
        auto& storage = storageFor<T>();
        return storage.emplace(entity.id(), std::forward<Args>(args)...);
    }

    template <typename T>
    bool has(Entity entity) const {
        if (!valid(entity)) return false;
        const auto it = storages_.find(componentType<T>());
        if (it == storages_.end()) return false;
        return static_cast<const Storage<T>*>(it->second.get())->has(entity.id());
    }

    template <typename T>
    T* tryGet(Entity entity) {
        if (!valid(entity)) return nullptr;
        const auto it = storages_.find(componentType<T>());
        if (it == storages_.end()) return nullptr;
        return static_cast<Storage<T>*>(it->second.get())->tryGet(entity.id());
    }

    template <typename T>
    T& get(Entity entity) {
        T* value = tryGet<T>(entity);
        assert(value);
        return *value;
    }

    template <typename T>
    void remove(Entity entity) {
        if (!valid(entity)) return;
        const auto it = storages_.find(componentType<T>());
        if (it != storages_.end())
            static_cast<Storage<T>*>(it->second.get())->remove(entity.id());
    }

private:
    struct IStorage {
        virtual ~IStorage() = default;
    };

    template <typename T>
    struct Storage final : IStorage {
        std::unordered_map<Entity::Id, T> values;

        template <typename... Args>
        T& emplace(Entity::Id id, Args&&... args) {
            auto [it, inserted] = values.try_emplace(id, std::forward<Args>(args)...);
            if (!inserted) it->second = T(std::forward<Args>(args)...);
            return it->second;
        }

        bool has(Entity::Id id) const { return values.find(id) != values.end(); }
        T* tryGet(Entity::Id id) {
            auto it = values.find(id);
            return it == values.end() ? nullptr : &it->second;
        }
        void remove(Entity::Id id) { values.erase(id); }
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

    std::vector<bool> alive_;
    std::vector<Entity::Generation> generations_;
    std::vector<Entity::Id> freeIds_;
    std::unordered_map<ComponentType, std::unique_ptr<IStorage>> storages_;
};

} // namespace storm::ecs
