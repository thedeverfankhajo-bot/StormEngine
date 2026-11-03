# ADR-0011: Initial Physics2D broadphase and collision scope

- Status: Accepted for the initial Physics2D vertical slice
- Date: 2026-11-03

## Context

StormEngine has an independent scalar Physics1D module and a planar Transform2D
layer, but no 2D physics implementation. The first 2D physics step must remain
independent of Core, Scene, Rendering, and Android/iOS SDKs while avoiding an
unbounded all-pairs scan on ordinary scenes.

## Decision

Create a separate `Storm::Physics2D` target. The first collision vocabulary is
circle and axis-aligned box colliders. Implement circle-circle, circle-box, and
box-box narrow-phase contacts, static/kinematic/dynamic body behavior,
gravity/force/impulse integration, restitution, friction, triggers, symmetric
collision filtering, and deterministic contact ordering.

Use a uniform spatial hash as the first broadphase. It is straightforward for
mobile-sized scenes and avoids scanning unrelated distant bodies. Make the
hash cell size, cells-per-body, total cell entries, candidate pair insertions,
body count, and contact count explicit bounded configuration values. Candidate
pairs are canonicalized, sorted, and deduplicated before narrow phase.

Simulation state is prepared in temporary storage and committed only after
broadphase, collision detection, resolution, and all limit checks succeed.

## Consequences

- 2D physics is independent of the 1D scalar world and does not depend on
  Transform3D.
- The initial module is intentionally axis-aligned; rotation and angular
  dynamics require a future contact/manifold design.
- Spatial hashing can create duplicate candidate insertions for bodies that
  share multiple cells. The candidate-insertion budget bounds that work.
- Dense clusters may exhaust candidate or contact budgets; the step returns an
  explicit error and leaves live simulation state unchanged.
- CCD, sleeping, persistent contact phases, queries, Scene integration,
  and device behavior remain unimplemented and must not be claimed.
