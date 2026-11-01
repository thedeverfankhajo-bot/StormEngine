# 1D Game Development (initial physics slice)

## Scope and status

StormEngine now has an initial one-dimensional physics model in the independent
`Storm::Physics1D` CMake target. Its gameplay coordinate is one scalar position;
velocity, acceleration, applied force, and impulses are also scalar values. The
simulation does not store Vector2 or Transform3D values and does not use the 2D
or 3D transform hierarchy.

An optional Scene bridge now maps scalar Transform1D components to PhysicsWorld1D bodies; see [1D Scene Integration](1D-SCENE-INTEGRATION.md). This is still a foundation, not a complete 1D game runtime. It
does not provide a 1D camera, game-state framework, input mapping, UI overlays,
renderer, asset pipeline, or a sample installable mobile game.

## Bodies and world

Use `PhysicsWorld1D` to own bodies. Bodies are referenced by handles containing
a world identity, slot index, and generation. Handles from another world and
handles invalidated by destruction are rejected. Reused slots receive a new
generation.

Each `BodyDefinition1D` declares:

- `Static`: does not integrate position or velocity.
- `Kinematic`: advances by its supplied velocity and is treated as immovable
  during collision resolution.
- `Dynamic`: integrates gravity and accumulated force using its positive mass.

A body is represented as a closed interval with a center position and positive
half-extent. Restitution is constrained to [0, 1]. Collision layer/mask fields
filter candidate pairs. A trigger overlap is reported but is not resolved.

## Fixed-step simulation

Call `PhysicsWorld1D::step(delta_seconds, gravity)` from an engine fixed-update
callback. The world does not own a clock or silently clamp its delta. It rejects
non-finite, non-positive, or over-limit deltas.

Dynamic bodies use semi-implicit Euler integration:

1. `velocity += (gravity + accumulated_force / mass) * delta_seconds`
2. `position += velocity * delta_seconds`

Accumulated forces are consumed once after a successful step. Impulses change a
dynamic body's velocity immediately. A failed step is transactional: body
positions, velocities, and pending forces are unchanged.

## Contacts and limits

Each successful step returns the contact pairs detected during that step.
Contact normals are directed from the first handle to the second and are always
-1 or +1. The reported point and penetration depth describe the overlap before
the positional correction. Contacts are returned deterministically in ascending
body-slot pair order.

The step also returns persistent `ContactEvent1D` transitions:

- `Begin`: the pair was not overlapping during the previous successful step.
- `Stay`: the same generation-checked pair is still overlapping.
- `End`: a previously overlapping pair is absent from the current step.

Triggers participate in the same phase sequence and retain `is_trigger` in the
event contact. An End event contains the last contact geometry reported for the
pair; its handles are historical and may already be stale after body destruction.
If a slot is reused before the next successful step, the old pair emits End and
the new generation emits Begin, in that order. Events are deterministic by
ascending body-slot pair order.

Contact history advances only when the entire step succeeds. A failed step
leaves body state, accumulated forces, and the previous contact set unchanged.

The default world limits are 256 bodies, 4096 contacts per step, and a maximum
step delta of 0.25 seconds. They are configurable. Exceeding the body/contact
budget returns a typed error rather than silently dropping bodies or contacts.

## Overlap queries

PhysicsWorld1D::overlap_query() performs a read-only query against the current
scalar body intervals and returns generation-checked BodyHandle1D values.
Query intervals are closed: a body that only touches a query boundary is
included. A zero half-extent is valid and represents a point query.

OverlapQuery1D::collision_mask is matched against each body's collision_layer;
a body's collision mask is used for body-to-body simulation filtering and is
not consulted by this query. Triggers are included by default and can be
excluded with include_triggers = false. Results are returned in ascending
body-slot order, independent of body position. The query does not mutate the
world and remains single-thread-affine.

Non-finite positions, negative/non-finite half-extents, and query bounds outside
the supported numeric range return typed errors. A stored body whose collider
bounds are outside the supported numeric range also causes an explicit error;
the query never silently omits that body.

## Raycasts

PhysicsWorld1D::raycast() casts a scalar ray through the current closed body
intervals and returns the nearest RaycastHit1D, or an empty optional when no
body is hit. The direction is a unit sign and must be exactly -1 or +1; the
maximum distance is finite, non-negative, and inclusive at the far endpoint.
The query matches collision_mask against each body's collision_layer and
includes triggers by default, with an option to exclude them.

If the origin is inside a body interval or exactly on its boundary, that body
is reported at distance zero. The reported normal is the entry normal opposing
the ray direction (-direction). Equal-distance hits are resolved by ascending
body-slot order. The query is read-only and single-thread-affine.

Invalid directions, non-finite inputs, overflowed ray endpoints, and invalid
stored collider bounds return typed errors rather than producing partial
results. A zero maximum distance is a point query against intervals containing
the origin.

## Deliberate limitations

- The first broadphase is a bounded O(n²) pair scan. It is a correctness-first
  baseline, not a performance claim or a large-scene solution.
- Collision is discrete; there is no continuous collision detection or
  tunneling prevention for fast-moving bodies.
- Bodies use one-dimensional intervals only. There are no rotations, surfaces,
  tangent friction, angular motion, joints, or 2D/3D colliders.
- Begin/Stay/End contact phases are available from successful PhysicsWorld1D
  steps, but they are not yet dispatched through the Core EventDispatcher.
  Sleeping and a higher-level gameplay event integration layer remain future work.
- PhysicsWorld1D itself remains independent of Scene. The optional
  PhysicsScene1D adapter performs explicit entity/body mapping and transform
  synchronization; it does not provide automatic component discovery.
- No Android/iOS device runtime, frame pacing, rendering, input, or packaging
  integration is claimed.

The 1D model intentionally remains independent of the 3D transform system. A
future presentation layer may draw the scalar world with 2D UI/rendering, but
that must not convert gameplay state into two-dimensional physics.
