#include <cassert>
#include <cmath>

#include "storm/core/World.hpp"

static bool near(float a, float b, float eps = 0.0001f) {
    return std::fabs(a - b) <= eps;
}

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

    const auto local = world.worldMatrix(entity);
    assert(near(local.m[0][3], 1.0f));
    assert(near(local.m[1][3], 2.0f));
    assert(near(local.m[2][3], 3.0f));

    world.removeTransform(entity);
    assert(world.transformCount() == 0);
    assert(world.tryGetTransform(entity) == nullptr);
    assert(near(world.worldMatrix(entity).m[0][0], 1.0f));

    const auto parent = world.createEntity();
    const auto child = world.createEntity();
    world.addTransform(parent).position = {10.0f, 0.0f, 0.0f};
    world.addTransform(child).position = {2.0f, 3.0f, 4.0f};

    assert(world.setParent(child, parent));
    assert(world.hasParent(child));
    assert(world.parentOf(child) == parent);

    const auto childWorld = world.worldMatrix(child);
    assert(near(childWorld.m[0][3], 12.0f));
    assert(near(childWorld.m[1][3], 3.0f));
    assert(near(childWorld.m[2][3], 4.0f));

    // Cached world transforms must refresh even when callers mutate the
    // public Transform fields directly.
    assert(near(world.worldMatrix(child).m[0][3], 12.0f));
    world.getTransform(parent).position.x = 20.0f;
    assert(near(world.worldMatrix(child).m[0][3], 22.0f));
    world.getTransform(child).position.y = 8.0f;
    assert(near(world.worldMatrix(child).m[1][3], 8.0f));

    assert(!world.setParent(parent, child));
    assert(world.parentOf(child) == parent);

    world.clearParent(child);
    assert(!world.hasParent(child));
    const auto unparented = world.worldMatrix(child);
    assert(near(unparented.m[0][3], 2.0f));

    const auto grandchild = world.createEntity();
    world.addTransform(grandchild).position = {1.0f, 1.0f, 1.0f};
    assert(world.setParent(child, parent));
    assert(world.setParent(grandchild, child));
    const auto nested = world.worldMatrix(grandchild);
    assert(near(nested.m[0][3], 23.0f));
    assert(near(nested.m[1][3], 9.0f));
    assert(near(nested.m[2][3], 5.0f));

    world.destroyEntity(parent);
    assert(!world.valid(parent));
    assert(!world.hasParent(child));
    assert(world.hasParent(grandchild));
    assert(world.parentOf(grandchild) == child);

    world.addTransform(entity);
    world.destroyEntity(entity);
    assert(!world.valid(entity));
    assert(world.transformCount() == 2);
    assert(world.tryGetTransform(entity) == nullptr);

    const auto replacement = world.createEntity();
    assert(world.valid(replacement));
    assert(replacement.id() == entity.id());
    assert(replacement.generation() != entity.generation());

    return 0;
}
