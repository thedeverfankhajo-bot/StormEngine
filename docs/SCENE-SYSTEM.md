# Scene and Entity System (initial implementation)

## Scope

The initial Core scene slice provides:

- A process-unique, non-zero SceneId allocated without wrapping.
- A move-only Scene owner.
- EntityHandle values containing scene identity, entity slot, and generation.
- Entity creation, destruction, name lookup, and rename operations.
- Generation changes on destruction so stale handles cannot become valid when a slot is reused.
- Scene-local, type-indexed component storage with typed add_component, try_get_component, and remove_component operations.
- Automatic component removal when an entity is destroyed.
- Typed Result failures for invalid, stale, cross-scene, duplicate, and missing-component operations.

## Usage

~~~cpp
#include "storm/core/scene.hpp"

struct Position final {
    float value{0.0F};
};

auto scene_result = Storm::Scene::create("Gameplay");
if (!scene_result) {
    // Handle scene_result.error().
    return;
}

Storm::Scene scene = std::move(scene_result).value();
auto entity_result = scene.create_entity("Player");
if (!entity_result) {
    // Handle entity_result.error().
    return;
}

const Storm::EntityHandle player = entity_result.value();
auto position_result = scene.add_component<Position>(player, Position{3.0F});
if (!position_result) {
    // Handle position_result.error().
    return;
}

position_result.value().get().value = 8.0F;
~~~

try_get_component<T>() returns a non-owning pointer and returns nullptr when the handle is invalid or the component does not exist. A returned component pointer/reference remains valid until that component is removed, its entity is destroyed, or the Scene is destroyed. Do not retain it across those operations.

## Identity and lifetime rules

- A handle is valid only for the Scene ID, entity slot, and generation that created it.
- Handles from another Scene are rejected even when slot and generation values happen to match.
- Destroying an entity invalidates its handle before the slot can be reused.
- If an entity generation reaches its maximum representable value, that slot is retired instead of wrapping and potentially reviving an old handle.
- Moving a Scene transfers its identity and owned data; existing handles remain valid in the destination. The moved-from Scene becomes invalid and cannot create entities.
- Scene and component storage operations are not thread-safe. Call them from one owning engine thread unless synchronization is added by a future, explicit runtime contract.

## Component constraints

Components are ordinary C++ object types with non-throwing destructors. Their constructors may accept arbitrary forwarded arguments. A Scene stores one component of each concrete C++ type per entity. Different Scene instances keep separate component stores.

The current implementation uses type-indexed storage and hash maps. It is an initial correctness-oriented component registry, not a cache-optimized archetype ECS. Iteration APIs, queries, system scheduling, component serialization, reflection, and editor metadata are not implemented.

## Explicit non-goals

This change does not implement:

- Parent/child scene hierarchy or transforms.
- Scene loading/unloading, activation, or transitions.
- Scene serialization or project persistence.
- 1D, 2D, or 3D math/physics/rendering.
- Component reflection, dynamic component types, or editor inspectors.
- Thread-safe concurrent Scene mutation.
- A stable/released public API.

The API is a development interface and may change. See API-VERSIONING.md.

## Validation

stormengine_scene_tests covers Scene identity, entity count and names, cross-scene handle rejection, stale-handle invalidation, slot reuse with generation changes, typed component storage, duplicate/missing component errors, component cleanup, and move ownership. CI runs this executable with the existing GCC, Clang, Apple Clang, and Clang ASan/UBSan host configurations. Android and iOS target builds compile the same Core library, but this does not establish on-device behavior.
