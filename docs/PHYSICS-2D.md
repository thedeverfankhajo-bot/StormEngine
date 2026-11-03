# 2D Physics (initial vertical slice)

## Scope and status

The independent `Storm::Physics2D` CMake target provides a bounded, fixed-step
2D rigid-body foundation. It is separate from `Storm::Physics1D`, Core, Scene,
and platform SDKs. It currently supports circles and axis-aligned boxes,
Static/Kinematic/Dynamic bodies, gravity, accumulated force, impulses,
restitution, Coulomb-style friction, triggers, symmetric layer/mask filtering,
contact output, persistent Begin/Stay/End transitions, and deterministic
body-slot ordering.

This is an initial physics slice, not a complete 2D engine. It does not yet
provide capsule/polygon/edge colliders, rotation or angular velocity, joints,
sleeping, continuous collision detection, rendering, or device-runtime
verification. Read-only overlap and raycast
queries are available for the current Circle/AABB shapes. The optional
Storm::PhysicsScene2D bridge supplies the initial Scene integration described
below; it does not add missing collision or rendering features.

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

## Contact resolution limits

The first solver pass is deliberately simple and deterministic. It applies one
normal impulse, a Coulomb-style tangential impulse, and positional correction
with a 0.005-unit slop and 80% correction factor. It is not an iterative
constraint solver; small residual penetration may remain after one step,
especially in dense multi-contact configurations. A later solver iteration
budget and CCD policy must be designed and benchmarked before treating this as
a production-grade general-purpose physics backend.

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

## Contact transition events

Each successful `step()` returns the current `contacts` plus a deterministic
`contact_events` stream. A newly detected pair emits `Begin`; a pair present
in the previous successful step emits `Stay`; and a pair that disappeared
emits `End` carrying the last contact snapshot. If a body slot is destroyed
and reused with a new generation while the pair remains geometrically
overlapping, the step emits `End` for the old handle followed by `Begin` for
the new handle.

Events are ordered by the pair's first and second slot indices. A failed step
does not update persistent contact history, so retrying the same state cannot
turn a pending `Begin` into `Stay` or lose an `End`. The event history is
single-thread-affine and bounded by the previous and current per-step contact
limits; event allocation failure aborts the step before body state is committed.
`ContactEvent2D` is the portable polling result; no asynchronous callback or
Scene event-dispatch integration is implied.

See [ADR-0022](ADR-0022-PHYSICS-2D-CONTACT-TRANSITIONS.md) for the decision.

## Overlap and raycast queries

The overlap_query API accepts a position and a Circle or axis-aligned Box query
shape. It returns every live body whose shape touches or overlaps the query, in
ascending body-slot order. The raycast API accepts a finite origin, a non-zero
direction (normalized internally), and a non-negative maximum distance. It
returns the nearest Circle or AABB hit, including the body handle, hit point,
unit normal, distance, and trigger flag. A ray beginning inside or on a
collider reports a zero-distance hit.

Both query APIs test the query mask against each body's collision layer and can
include or exclude triggers. They are read-only and single-thread-affine. The
implementation uses a stable O(n) scan bounded by maximum_bodies (512 by
default); simulation still uses the spatial-hash broadphase. Invalid query
values return InvalidArgument, a moved-from world returns InvalidState, and
allocation failure in overlap-result storage returns ResourceExhausted.

## Transaction and threading contract

`PhysicsWorld2D` is single-thread-affine. A step works on temporary body state,
constructs the broadphase and contacts, checks configured budgets, and commits
positions, velocities, and force consumption only after the complete step
succeeds. A failed step leaves live body state and accumulated forces unchanged.
Body handles contain a process-unique world ID, slot index, and generation;
stale and foreign handles are rejected. Slots at generation exhaustion are
retired instead of wrapping.

## Scene integration

The optional Storm::PhysicsScene2D target connects the independent world to
Scene entities without making Core depend on physics. Each attached entity must
be alive, unparented, and own a finite Transform2D. Its local
Transform2D::translation is authoritative when the body is attached; the body
definition's initial position is ignored. Successful steps synchronize the
body's position back to translation while preserving rotation and scale.

The bridge intentionally does not perform parent-space conversion. Parenting a
bound entity is rejected before the physics world advances. The current physics
world has no angular dynamics or rotated colliders, so Transform2D rotation and
scale do not alter collider geometry. Scene translation is stored as float,
while simulation state is double; the bridge rejects coordinate limits that
cannot be represented by finite Transform2D values. The bridge and Scene
structural mutations are single-thread-affine. Destroyed entities and removed
Transform2D components are retired on the next adapter step invocation.
Removing and re-adding Transform2D between steps is not observable; detach
explicitly before replacing the component.

See ADR-0012 for the integration decision and explicit limitations.

## Validation boundary

The host test executable covers handle lifetime, validation, integration,
circle and box collision response, friction, triggers, filtering, spatial-hash
candidate reduction, overlap queries, raycasts, configured resource limits,
and transactional failure.
The host Physics2D suite covers the world contract; the separate Scene adapter
has its own integration tests. Android arm64-v8a and iOS device arm64 builds
compile both platform-neutral targets. Cross-compilation is not device-runtime
verification. There is no 2D gameplay sample, native renderer, or
physical-device physics validation yet.
