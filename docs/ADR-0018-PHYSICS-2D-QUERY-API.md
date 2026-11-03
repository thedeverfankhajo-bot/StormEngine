# ADR-0018: Bounded Physics2D Query API

- Status: Accepted
- Date: 2026-11-03
- Scope: Read-only overlap and raycast queries for the initial Circle/AABB Physics2D slice

## Context

The initial PhysicsWorld2D simulation slice already detects contacts between
circles and axis-aligned boxes. Gameplay systems also need read-only overlap and
raycast queries for sensors, aiming, selection, and simple character logic.
The world has a configurable body limit (512 by default) and remains
single-thread-affine.

## Decision

Add overlap_query and raycast to PhysicsWorld2D without introducing another
spatial index.

- Overlap queries accept a query position and the existing Circle/AABB collider
  variant. They reuse the simulation narrow-phase collision routines and return
  matching handles in ascending slot order.
- Raycasts accept any finite non-zero direction vector and normalize it
  internally. They use exact ray-circle intersection and a slab intersection
  against axis-aligned box bounds, returning only the nearest hit.
- Both APIs are read-only, single-thread-affine, filter against body collision
  layers, and optionally include triggers.
- Query work is bounded by maximum_bodies. The O(n) scan is intentional for
  this bounded first slice; the simulation's spatial hash remains dedicated to
  pair generation. A separate query acceleration structure can be evaluated
  only after benchmarks demonstrate a need.
- Invalid user input returns InvalidArgument; moved-from worlds return
  InvalidState. Overlap result allocation failures return ResourceExhausted
  rather than truncating results silently.

## Consequences

The 2D gameplay layer can query current body state without mutating simulation
state or depending on Scene, Core, Android, iOS, or rendering APIs. The API does
not imply support for rotated boxes, polygons, capsules, persistent contacts,
CCD, or physics-device verification. The Scene bridge may expose these methods
later if concrete entity-oriented use cases justify the extra surface area.
