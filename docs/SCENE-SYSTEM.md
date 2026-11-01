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
- Explicit optional parent links and ordered direct-child lists.
- Reparenting with self-parent and cycle rejection, including stale/cross-Scene handle validation.
- Detaching by assigning a null parent.
- Consistent two-way hierarchy updates when reparenting or destroying entities.
- Parent destruction orphans direct children while preserving each child's own subtree.
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

## Entity hierarchy

Use set_parent(child, parent) to attach or reparent an entity. Pass std::nullopt to detach it. parent_of(entity) returns an optional parent handle. children_of(entity) returns a read-only span of direct children in sibling insertion order.

The implementation rejects self-parenting, ancestry cycles, stale handles, and handles from another Scene. Reparenting updates both the old and new parent's child lists. Repeating the current parent assignment is idempotent and does not duplicate the child.

Destroying a child removes its link from its parent. Destroying a parent invalidates that parent, removes its link from its own parent, and detaches its direct children. Those children and their descendants remain alive; each direct child becomes a root while retaining its own subtree.

The span returned by children_of() is non-owning. Do not retain it across a hierarchy mutation, entity destruction, Scene move/assignment, or Scene destruction. Hierarchy changes are single-thread-affine, like the rest of Scene.

The hierarchy is consumed by the separate [Transform System](TRANSFORM-SYSTEM.md), which provides optional local Transform3D components and on-demand world matrices. It does not provide sibling reordering or integrate with a renderer or physics system.

## Identity and lifetime rules

- A handle is valid only for the Scene ID, entity slot, and generation that created it.
- Handles from another Scene are rejected even when slot and generation values happen to match.
- Destroying an entity invalidates its handle before the slot can be reused.
- If an entity generation reaches its maximum representable value, that slot is retired instead of wrapping and potentially reviving an old handle.
- Moving a Scene transfers its identity and owned data; existing handles remain valid in the destination. The moved-from Scene becomes invalid and cannot create entities.
- Scene and component storage operations are not thread-safe. Call them from one owning engine thread unless synchronization is added by a future, explicit runtime contract.


## Iteration

- `for_each_entity(callback)` visits live entities in ascending entity-slot order and passes an `EntityHandle` plus a non-owning `std::string_view` of the entity name.
- `for_each_component<T>(callback)` visits only live entities that own component `T`. The mutable overload passes `T&`; the const overload passes `const T&`.
- Both APIs return the number of callback invocations as `Result<std::size_t>`. A moved-from Scene returns `InvalidState`; an absent component store returns a successful count of zero.
- Iteration order is based on entity slot index, not creation time or hash-table order. A reused slot is visited at its slot position.
- Iteration is synchronous and single-thread-affine. Callbacks must not structurally mutate the Scene, destroy the Scene, or retain non-owning name/component references beyond their documented lifetimes. Exceptions thrown by callbacks propagate to the caller.

Component iteration currently scans the entity-slot array and performs a component-store lookup for each live entity. It favors deterministic behavior and a small API over archetype-ECS query performance. Multi-component queries, cached views, and a system scheduler remain future work.

## Component constraints

Components are ordinary C++ object types with non-throwing destructors. Their constructors may accept arbitrary forwarded arguments. A Scene stores one component of each concrete C++ type per entity. Different Scene instances keep separate component stores.

The current implementation uses type-indexed storage and hash maps. It is an initial correctness-oriented component registry, not a cache-optimized archetype ECS. Multi-component queries, system scheduling, reflection, and editor metadata are not implemented. Component values can be persisted through the separate [Scene Serialization](SCENE-SERIALIZATION.md) module only when an explicit stable-ID/versioned codec is supplied.

## Explicit non-goals

This change does not implement:

- Transform caching or dirty propagation.
- Scene loading/unloading, activation, or transitions.
- Direct filesystem I/O, atomic file replacement, or automatic component-schema migration.
- 1D, 2D, or 3D math/physics/rendering.
- Component reflection, dynamic component types, or editor inspectors.
- Thread-safe concurrent Scene mutation.
- A stable/released public API.

The API is a development interface and may change. See API-VERSIONING.md.

## Validation

stormengine_scene_tests covers Scene identity, entity count and names, cross-scene handle rejection, stale-handle invalidation, slot reuse with generation changes, typed component storage, duplicate/missing component errors, component iteration, entity iteration, component cleanup, move ownership, parent/child consistency, reparenting, detach, cycle rejection, stale/foreign parents, destruction cleanup, and move-preserved hierarchy. CI runs this executable with the existing GCC, Clang, Apple Clang, and Clang ASan/UBSan host configurations. Android and iOS target builds compile the same Core library, but this does not establish on-device behavior.
