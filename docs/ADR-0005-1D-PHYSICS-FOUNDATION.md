# ADR-0005: Establish a Scalar 1D Physics World

- Status: Accepted
- Date: 2026-11-01

## Context

StormEngine targets distinct 1D, 2D, and 3D gameplay models. The existing
Scene/Entity registry and Transform3D system are general-purpose/3D foundations;
they do not constitute a true one-dimensional world. The first 1D implementation
must therefore use scalar state and must not encode gameplay coordinates in
Vector2 or Transform3D.

## Decision

Add an independent `Storm::Physics1D` target with a `PhysicsWorld1D` owner.
The initial slice includes world-scoped generation-checked body handles,
Static/Kinematic/Dynamic body types, scalar position and velocity, accumulated
force, impulse, gravity, interval overlap detection, trigger contacts,
restitution-based one-axis impulse resolution, collision layer/mask filtering,
read-only filtered closed-interval overlap queries, nearest-hit scalar raycasts with deterministic tie handling, persistent Begin/Stay/End contact transitions, and explicit body/contact/time-step limits.

The world is stepped by the existing caller-owned fixed-update phase. Each
successful step consumes accumulated forces. The step computes against staged
temporary state and commits only after integration, contact generation, event
transition derivation, and all configured limits succeed. Contact phase history
is keyed by generation-checked body pairs: a pair emits Begin on first overlap,
Stay while continuously overlapping, and End when absent from a later
successful step. Slot reuse produces End for the old generation before Begin
for the replacement. A failed step does not advance contact history.

## Alternatives considered

- Reusing Transform3D or Vector2: rejected because it would obscure the
  dimensional contract and make 1D physics a constrained 2D/3D facade.
- Integrating physics directly into Core Scene: rejected because physics should
  remain an optional system and Core must not acquire a mandatory simulation
  dependency.
- Adding a third-party physics engine immediately: deferred because the initial
  1D interval model is small and self-contained. External engines will be
  reconsidered for 2D/3D only after mobile compatibility, licensing, and
  performance evaluation.

## Consequences

- The first implementation is intentionally discrete and correctness-oriented.
- The broadphase is O(n²) with a conservative configurable body ceiling; it
  requires profiling before increasing limits or claiming scale.
- Friction, continuous collision detection, joints, sleeping, and Scene
  integration remain out of scope. Contact phase events are returned by
  PhysicsWorld1D but are not dispatched through Core EventDispatcher.
- Host behavior tests and Android/iOS target compilation are required before
  reporting the implementation as tested. Physical-device validation remains a
  separate requirement. The scalar raycast contract accepts only unit directions
  of -1 or +1 and returns the nearest interval entry; it does not add a 2D/3D
  vector-ray API.

## Evidence

The implementation, tests, and target-build evidence are tracked in
[1D Game Development](1D-GAME-DEVELOPMENT.md), [Testing](TESTING.md), and the
[Compatibility Matrix](COMPATIBILITY.md).
