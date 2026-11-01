# 1D Scene and Physics Integration

## Status and scope

This layer connects the scalar PhysicsWorld1D simulation to the generation-
checked Scene/Entity registry. It introduces a true scalar Transform1D
component and an optional PhysicsScene1D adapter.

The adapter is a separate CMake target named storm_physics_scene1d. Core does
not depend on physics, and the standalone Physics1D target does not depend on
Scene. Applications opt into the bridge only when they need both.

This is not a complete 1D game runtime. A 1D camera, input mapping, animation,
in-game UI, renderer, asset pipeline, and installable mobile game are not
included.

## Scalar transform

Transform1D stores one finite scalar position. It does not wrap Vector2,
Vector3, or Transform3D and does not add hidden axes. The gameplay model remains
one-dimensional even when a later renderer chooses a two-dimensional
presentation layer.

## Attaching a body

Create a Scene entity and add Transform1D before calling PhysicsScene1D::attach.
The entity's Transform1D position is authoritative at attachment time;
BodyDefinition1D position is intentionally ignored. This prevents two
competing initial positions.

PhysicsScene1D owns its PhysicsWorld1D and is bound to the SceneId supplied at
creation. It retains generation-checked EntityHandle/BodyHandle1D pairs.
Passing another Scene or a stale entity is rejected with a typed error.

## Fixed-step use

Call PhysicsScene1D::step from the engine fixed-update callback. After a
successful physics step, positions are copied back into each mapped
Transform1D. The returned contacts and contact events contain physics body
handles; use entity_for_body to resolve a still-attached body to its Scene
entity.

Use set_position for a deliberate teleport. Use set_velocity, apply_force, and
apply_impulse for explicit simulation changes. Directly changing Transform1D
while a body is attached does not teleport the physics body; a subsequent
successful step writes the physics position back to the component.

## Ownership and lifecycle

- Scene moves preserve the SceneId and keep the adapter binding valid.
- Entity handles remain generation-checked. Destroyed entities and entities
  missing Transform1D are retired before the next adapter step.
- Removing Transform1D and re-adding it before the next adapter step cannot be
  observed by this polling adapter. Explicitly detach before replacing a
  Transform1D component.
- detach explicitly destroys the owned body and removes its mapping.
- The adapter is single-thread-affine. Scene structure must not be mutated
  concurrently with adapter calls.
- Failed PhysicsWorld1D steps do not copy simulated positions back to Scene.
- The mapping currently uses linear lookup. This is a correctness baseline,
  not a large-scene performance claim.

## Deliberate limits

The adapter does not add a Scene observer, automatic component discovery,
reflection, or serialization codec registration. Physics contact events are
returned by the existing 1D world and are not dispatched through Core's
EventDispatcher. The underlying collision broadphase remains the bounded
O(n squared) pair scan documented in 1D-GAME-DEVELOPMENT.md.

No renderer, camera, touch input, in-game UI, Android/iOS gameplay host, or
physical-device runtime validation is provided by this integration.
