# 2D Physics (initial vertical slice)

## Scope and status

The independent `Storm::Physics2D` CMake target provides a bounded, fixed-step
2D rigid-body foundation. It is separate from `Storm::Physics1D`, Core, Scene,
and platform SDKs. It currently supports circles and axis-aligned boxes,
Static/Kinematic/Dynamic bodies, gravity, accumulated force, impulses,
restitution, Coulomb-style friction, triggers, symmetric layer/mask filtering,
contact output, and deterministic body-slot ordering.

This is an initial physics slice, not a complete 2D engine. It does not yet
provide capsule/polygon/edge colliders, rotation or angular velocity, joints,
sleeping, continuous collision detection, overlap/raycast queries, persistent
Begin/Stay/End events, Scene integration, rendering, or device-runtime
verification.

## Coordinates and body behavior

The simulation uses double-precision `Vector2` values. +X points right and +Y
points up. Bodies are not rotated. A `CircleCollider2D` is defined by a positive
radius; a `BoxCollider2D` is defined by positive half-extents and remains
axis-aligned.

- Static bodies do not integrate or respond to impulses.
- Kinematic bodies advance using their assigned velocity and have zero inverse
  mass during collision resolution.
- Dynamic bodies require a positive mass with a representable bounded inverse.
  They integrate gravity and accumulated force using semi-implicit Euler.
- Forces are consumed once after a successful step. Impulses change dynamic
  velocity immediately.
- Triggers generate contacts but do not resolve or alter body state.

Collision filtering is symmetric: both bodies' layer/mask tests must pass.
Restitution and friction are finite values in the inclusive range [0, 1].
Normal impulses use the smaller restitution; tangential friction is bounded by
the product of the normal impulse and the geometric mean of the two friction
coefficients.

## Broadphase and limits

The broadphase uses a uniform spatial hash. Each body's world AABB is inserted
into every overlapped cell; canonical candidate pairs are sorted and
deduplicated before narrow phase. Narrow phase currently supports circle-circle,
circle-box, and axis-aligned box-box contacts.

Default limits are:

| Limit | Default |
|---|---:|
| Bodies | 512 |
| Contacts per step | 4,096 |
| Spatial-hash cell entries | 16,384 |
| Candidate pair insertions | 32,768 |
| Cells touched by one body | 256 |
| Cell size | 4.0 world units |
| Maximum fixed step | 0.25 seconds |
| Maximum absolute position/velocity component | 1e9 |

Every configured capacity is checked. Exceeding a budget returns a typed
`ResourceExhausted` error; the implementation does not silently drop bodies,
pairs, or contacts. Coordinates that cannot be mapped safely to signed
64-bit cell indices are rejected.

## Transaction and threading contract

`PhysicsWorld2D` is single-thread-affine. A step works on temporary body state,
constructs the broadphase and contacts, checks configured budgets, and commits
positions, velocities, and force consumption only after the complete step
succeeds. A failed step leaves live body state and accumulated forces unchanged.
Body handles contain a process-unique world ID, slot index, and generation;
stale and foreign handles are rejected. Slots at generation exhaustion are
retired instead of wrapping.

## Validation boundary

The host test executable covers handle lifetime, validation, integration,
circle and box collision response, friction, triggers, filtering, spatial-hash
candidate reduction, configured resource limits, and transactional failure.
Android arm64-v8a and iOS device arm64 builds compile this same platform-neutral
target. Cross-compilation is not device-runtime verification. There is no
2D gameplay sample, native renderer, or physical-device physics validation yet.
