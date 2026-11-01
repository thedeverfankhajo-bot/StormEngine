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
and explicit body/contact/time-step limits.

The world is stepped by the existing caller-owned fixed-update phase. Each
successful step consumes accumulated forces. The step computes against staged
temporary state and commits only after integration, contact generation, and
all configured limits succeed.

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
- Friction, continuous collision detection, joints, sleeping, persistent
  contact phases, and Scene integration remain out of scope.
- Host behavior tests and Android/iOS target compilation are required before
  reporting the implementation as tested. Physical-device validation remains a
  separate requirement.

## Evidence

The implementation, tests, and target-build evidence are tracked in
[1D Game Development](1D-GAME-DEVELOPMENT.md), [Testing](TESTING.md), and the
[Compatibility Matrix](COMPATIBILITY.md).
