# 1D Game Development (initial physics slice)

## Scope and status

StormEngine now has an initial one-dimensional physics model in the independent
`Storm::Physics1D` CMake target. Its gameplay coordinate is one scalar position;
velocity, acceleration, applied force, and impulses are also scalar values. The
simulation does not store Vector2 or Transform3D values and does not use the 2D
or 3D transform hierarchy.

The current implementation is a foundation, not a complete 1D game runtime. It
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

The default world limits are 256 bodies, 4096 contacts per step, and a maximum
step delta of 0.25 seconds. They are configurable. Exceeding the body/contact
budget returns a typed error rather than silently dropping bodies or contacts.

## Deliberate limitations

- The first broadphase is a bounded O(n²) pair scan. It is a correctness-first
  baseline, not a performance claim or a large-scene solution.
- Collision is discrete; there is no continuous collision detection or
  tunneling prevention for fast-moving bodies.
- Bodies use one-dimensional intervals only. There are no rotations, surfaces,
  tangent friction, angular motion, joints, or 2D/3D colliders.
- Contacts are per-step results. Persistent Begin/Stay/End contact tracking,
  sleeping, and a full event integration layer are future work.
- PhysicsWorld1D is single-thread-affine and currently does not mutate Scene
  components automatically.
- No Android/iOS device runtime, frame pacing, rendering, input, or packaging
  integration is claimed.

The 1D model intentionally remains independent of the 3D transform system. A
future presentation layer may draw the scalar world with 2D UI/rendering, but
that must not convert gameplay state into two-dimensional physics.
