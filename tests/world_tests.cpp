#include <cassert>

#include "storm/core/World.hpp"

int main() {
    storm::core::World world;

    const auto entity = world.createEntity();
    assert(world.valid(entity));
    assert(world.transformCount() == 0);

    auto& transform = world.addTransform(entity);
    transform.position = {1.0f, 2.0f, 3.0f};
    assert(world.transformCount() == 1);
    assert(world.getTransform(entity).position.x == 1.0f);
    assert(world.getTransform(entity).position.y == 2.0f);
    assert(world.getTransform(entity).position.z == 3.0f);

    world.removeTransform(entity);
    assert(world.transformCount() == 0);
    assert(world.tryGetTransform(entity) == nullptr);

    world.addTransform(entity);
    world.destroyEntity(entity);
    assert(!world.valid(entity));
    assert(world.transformCount() == 0);
    assert(world.tryGetTransform(entity) == nullptr);

    const auto replacement = world.createEntity();
    assert(world.valid(replacement));
    assert(replacement.id() == entity.id());
    assert(replacement.generation() != entity.generation());

    return 0;
}
