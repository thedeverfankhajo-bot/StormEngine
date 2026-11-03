# ADR-0012: Physics2D Scene Integration

- Status: Accepted
- Date: 2026-11-03
- Scope: optional Storm::PhysicsScene2D bridge

## Context

Storm::Physics2D is intentionally independent of Core and Scene. The existing
PhysicsScene1D adapter demonstrates that Scene integration can be provided as
an optional target without introducing a reverse dependency from Core to
physics. The initial Physics2D world uses double-precision positions, while
Math::Transform2D stores translation in floats. Its first collider set also
contains only circles and axis-aligned boxes; angular motion and rotated shapes
are not implemented.

## Decision

Add a separate Storm::PhysicsScene2D library that depends on Physics2D, Core,
and Math. It binds an entity's Transform2D::translation to one Physics2D body
using world-local generation-checked body handles.

The adapter's contract is deliberately bounded:

- An entity must be alive, belong to the bound Scene, and own a finite
  Transform2D.
- Physics-bound entities must be unparented. Parent-space conversion and
  synchronization are deferred until a complete world/local transform policy
  exists.
- The Transform2D translation is authoritative at attachment; the body
  definition's initial position is ignored.
- Only translation is synchronized. Rotation and scale remain untouched and
  do not rotate or resize the current axis-aligned physics colliders.
- The bridge requires the Physics2D coordinate limit to fit finite float
  Transform2D translation values. Physics simulation itself remains double
  precision.
- Calls and Scene structural changes are single-thread-affine. Removed
  entities/components are retired on the next adapter step invocation.
  Removing and re-adding Transform2D between steps is not observable; callers
  must detach before replacing the component.
- The adapter remains outside Core and Physics2D, preserving dependency
  direction and allowing the physics world to be used without Scene.

## Consequences

The first 2D gameplay slice can synchronize body positions with Scene entities
and reuse existing Scene lifetime validation without introducing platform or
renderer dependencies. Parented physics bodies, rotated colliders, angular
dynamics, and higher-precision Scene transforms remain explicitly unsupported
rather than silently approximated.

The adapter is covered by host integration tests and Android/iOS target
cross-compilation. Those checks do not constitute physical-device runtime
verification.
