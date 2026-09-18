#include <cassert>
#include <cstdint>
#include <stdexcept>
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
    assert(set.size() == 3 && set.contains(10) && set.position(20) == 1);
    set.erase(20);
    assert(!set.contains(20) && set.size() == 2 && set.contains(30) && set.position(30) == 1);
}
void testComponentStorageSwapBack() {
    storm::ecs::ComponentStorage<Position> storage;
    storage.emplace(1, 10, 20); storage.emplace(2, 30, 40); storage.emplace(3, 50, 60);
    storage.remove(2);
    assert(storage.size() == 2 && storage.tryGet(2) == nullptr && storage.tryGet(3) != nullptr);
    assert(storage.tryGet(3)->x == 50 && storage.entityAt(1) == 3);
}
void testRegistryGeneration() {
    storm::ecs::Registry registry;
    const auto first = registry.create(); registry.destroy(first);
    const auto reused = registry.create();
    assert(reused.id() == first.id() && reused.generation() != first.generation());
    assert(registry.valid(reused) && !registry.valid(first));
}
void testRegistryComponents() {
    storm::ecs::Registry registry; const auto entity = registry.create();
    registry.emplace<Position>(entity, 1, 2); registry.emplace<Velocity>(entity, 3, 4);
    assert(registry.has<Position>(entity) && registry.has<Velocity>(entity));
    assert(registry.get<Position>(entity).x == 1 && registry.get<Velocity>(entity).y == 4);
    registry.remove<Velocity>(entity); assert(!registry.has<Velocity>(entity));
    registry.destroy(entity); assert(!registry.has<Position>(entity));
}
void testRegistryGetThrowsInsteadOfUsingReleaseAssert() {
    storm::ecs::Registry registry; const auto entity = registry.create();
    bool threw = false;
    try { (void)registry.get<Position>(entity); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
}
void testQueryUsesMatchingEntities() {
    storm::ecs::Registry registry;
    const auto a = registry.create(), b = registry.create(), c = registry.create();
    registry.emplace<Position>(a, 1, 0); registry.emplace<Velocity>(a, 10, 0);
    registry.emplace<Position>(b, 2, 0);
    registry.emplace<Position>(c, 3, 0); registry.emplace<Velocity>(c, 30, 0); registry.emplace<Tag>(c, 99);
    std::vector<storm::ecs::Entity::Id> visited;
    registry.each<Position, Velocity>([&](auto entity, Position& position, Velocity& velocity) { visited.push_back(entity.id()); position.x += velocity.x; });
    assert(visited.size() == 2 && registry.get<Position>(a).x == 11 && registry.get<Position>(c).x == 33 && registry.get<Position>(b).x == 2);
}
void testQueryKeepsCallableUsableAcrossMatches() {
    storm::ecs::Registry registry;
    const auto a = registry.create(), b = registry.create();
    registry.emplace<Position>(a, 1, 0);
    registry.emplace<Position>(b, 2, 0);
    std::size_t calls = 0;
    auto callback = [&](auto, Position&) { ++calls; };
    registry.each<Position>(std::move(callback));
    assert(calls == 2);
}

void testQueryUsesSmallestStorage() {
    storm::ecs::Registry registry;
    const auto a = registry.create(), b = registry.create(), c = registry.create();
    registry.emplace<Position>(a, 1, 0); registry.emplace<Velocity>(a, 10, 0);
    registry.emplace<Position>(b, 2, 0); registry.emplace<Velocity>(b, 20, 0);
    registry.emplace<Position>(c, 3, 0);
    std::vector<storm::ecs::Entity::Id> visited;
    registry.each<Position, Velocity>([&](auto entity, Position&, Velocity&) { visited.push_back(entity.id()); });
    assert(visited.size() == 2 && visited[0] != visited[1]);

    storm::ecs::Registry smallFirst;
    const auto e1 = smallFirst.create(), e2 = smallFirst.create(), e3 = smallFirst.create();
    smallFirst.emplace<Position>(e1, 1, 0); smallFirst.emplace<Position>(e2, 2, 0); smallFirst.emplace<Position>(e3, 3, 0);
    smallFirst.emplace<Velocity>(e1, 10, 0);
    std::size_t calls = 0;
    smallFirst.each<Position, Velocity>([&](auto, Position&, Velocity&) { ++calls; });
    assert(calls == 1);
}
}
int main() {
    testSparseSet(); testComponentStorageSwapBack(); testRegistryGeneration(); testRegistryComponents();
    testRegistryGetThrowsInsteadOfUsingReleaseAssert();
    testQueryUsesMatchingEntities();
    testQueryKeepsCallableUsableAcrossMatches();
    testQueryUsesSmallestStorage();
    return 0;
}
