#include <cassert>
#include <cstdint>
#include <vector>

#include "storm/ecs/ComponentStorage.hpp"
#include "storm/ecs/Registry.hpp"
#include "storm/ecs/SparseSet.hpp"

namespace {

struct Position { int x{}; int y{}; };
struct Velocity { int x{}; int y{}; };
struct Tag { int value{}; };

void testSparseSet() {
    storm::ecs::SparseSet set;
    set.insert(10); set.insert(20); set.insert(30);
    assert(set.size() == 3);
    assert(set.contains(10));
    assert(set.position(20) == 1);
    set.erase(20);
    assert(!set.contains(20));
    assert(set.size() == 2);
    assert(set.contains(30));
    assert(set.position(30) == 1);
}

void testComponentStorageSwapBack() {
    storm::ecs::ComponentStorage<Position> storage;
    storage.emplace(1, 10, 20); storage.emplace(2, 30, 40); storage.emplace(3, 50, 60);
    storage.remove(2);
    assert(storage.size() == 2);
    assert(storage.tryGet(2) == nullptr);
    assert(storage.tryGet(3) != nullptr);
    assert(storage.tryGet(3)->x == 50);
    assert(storage.entityAt(1) == 3);
}

void testRegistryGeneration() {
    storm::ecs::Registry registry;
    const auto first = registry.create();
    assert(registry.valid(first));
    registry.destroy(first);
    assert(!registry.valid(first));
    const auto reused = registry.create();
    assert(reused.id() == first.id());
    assert(reused.generation() != first.generation());
    assert(registry.valid(reused));
    assert(!registry.valid(first));
}

void testRegistryComponents() {
    storm::ecs::Registry registry;
    const auto entity = registry.create();
    registry.emplace<Position>(entity, 1, 2);
    registry.emplace<Velocity>(entity, 3, 4);
    assert(registry.has<Position>(entity));
    assert(registry.has<Velocity>(entity));
    assert(registry.get<Position>(entity).x == 1);
    assert(registry.get<Velocity>(entity).y == 4);
    registry.remove<Velocity>(entity);
    assert(!registry.has<Velocity>(entity));
    registry.destroy(entity);
    assert(!registry.has<Position>(entity));
}

void testQueryUsesMatchingEntities() {
    storm::ecs::Registry registry;
    const auto a = registry.create();
    const auto b = registry.create();
    const auto c = registry.create();
    registry.emplace<Position>(a, 1, 0); registry.emplace<Velocity>(a, 10, 0);
    registry.emplace<Position>(b, 2, 0);
    registry.emplace<Position>(c, 3, 0); registry.emplace<Velocity>(c, 30, 0); registry.emplace<Tag>(c, 99);
    std::vector<storm::ecs::Entity::Id> visited;
    registry.each<Position, Velocity>([&](auto entity, Position& position, Velocity& velocity) {
        visited.push_back(entity.id()); position.x += velocity.x;
    });
    assert(visited.size() == 2);
    assert(registry.get<Position>(a).x == 11);
    assert(registry.get<Position>(c).x == 33);
    assert(registry.get<Position>(b).x == 2);
}

void testQueryWhenFirstComponentIsNotSmallest() {
    storm::ecs::Registry registry;
    const auto a = registry.create();
    const auto b = registry.create();
    const auto c = registry.create();
    registry.emplace<Position>(a, 1, 0); registry.emplace<Velocity>(a, 10, 0);
    registry.emplace<Position>(b, 2, 0); registry.emplace<Velocity>(b, 20, 0);
    registry.emplace<Position>(c, 3, 0); // Position is intentionally the larger storage.
    std::vector<storm::ecs::Entity::Id> visited;
    registry.each<Position, Velocity>([&](auto entity, Position&, Velocity&) { visited.push_back(entity.id()); });
    assert(visited.size() == 2);
    assert(visited[0] == a.id() || visited[0] == b.id());
    assert(visited[1] == a.id() || visited[1] == b.id());
    assert(visited[0] != visited[1]);
}

} // namespace

int main() {
    testSparseSet();
    testComponentStorageSwapBack();
    testRegistryGeneration();
    testRegistryComponents();
    testQueryUsesMatchingEntities();
    testQueryWhenFirstComponentIsNotSmallest();
    return 0;
}
