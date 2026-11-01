# ADR-0009: Keep 1D Scene Integration Optional

- Date: 2026-11-01
- Status: Accepted

## Context

StormEngine has a generation-checked Scene/Entity component registry and an
independent scalar PhysicsWorld1D. The physics world intentionally depends
only on Foundation; Core must remain independent of optional simulation
subsystems. The first usable 1D gameplay slice needs a scalar transform on
entities and a defined synchronization boundary between Scene and physics.

## Decision

- Add Math::Transform1D, whose gameplay position is one finite scalar. It does
  not use Vector2 or Transform3D.
- Add a separate Storm::PhysicsScene1D target depending on Core, Math, and
  Physics1D.
- Bind each adapter instance to one SceneId and retain generation-checked
  EntityHandle/BodyHandle1D pairs.
- Require Transform1D when attaching a body. Its position is authoritative at
  attachment; successful fixed steps copy simulated positions back.
- Provide explicit teleport, velocity, force, impulse, detach, snapshot, and
  body/entity lookup operations.
- Retire mappings when an entity or its Transform1D component is removed,
  before the next adapter step.
- Keep calls single-thread-affine and reject use with a different Scene.
- Build and test the adapter on host, Android arm64, and iOS device arm64 in CI.

## Alternatives considered

1. Put physics directly inside Core: rejected because Core would gain a
   mandatory dependency on an optional gameplay subsystem.
2. Make Physics1D depend on Scene: rejected because the scalar physics world is
   useful without entities and should remain independently testable.
3. Use Transform3D or Vector2 with one axis fixed: rejected because it weakens
   the dimensional contract and makes 1D a constrained 2D/3D facade.
4. Add automatic component discovery: deferred. Explicit attach/detach keeps
   ownership and synchronization visible. The initial polling cleanup also
   means a removed-and-readded Transform1D between steps must be explicitly
   detached before replacement.

## Consequences

- Scene and Physics1D remain independently reusable.
- The integration adds an O(n) entity/body mapping table; it is not a large-
  scene performance claim.
- Component removal is detected at the next adapter step; no Scene observer
  or hidden lifetime callback is introduced.
- Physics remains single-thread-affine and is called from the fixed-update
  phase by the application.
- The 1D renderer, camera, touch input, UI, and mobile game host remain future
  work.
- Transform1D persistence is not included. Scene archives require explicit
  component codecs and must register a stable ID/schema before serializing this
  component.

## Validation

Unit tests cover component requirements, stale/foreign handles, position
synchronization, explicit teleport/force/impulse, lookup and detach behavior,
cleanup, move semantics, and failed-step behavior. Host CI and Android/iOS
target-build results must be reviewed separately from physical-device runtime
verification.
