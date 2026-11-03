# 3D Geometry Math (initial vertical slice)

## Scope and status

The platform-neutral `Storm::Math` target now includes validated 3D geometry
primitives alongside `Vector3`, `Quaternion`, `Matrix4`, and `Transform3D`:

- `Aabb3D`: finite axis-aligned minimum/maximum bounds, inclusive containment
  and overlap, checked construction from center/half-extents, and union.
- `Sphere3D`: finite center and non-negative radius, including point spheres.
- `Plane3D`: normalized normal and signed offset, built from a point and normal.
- `Ray3D`: finite origin and normalized direction created through a checked factory.
- Ray queries against AABBs, spheres, and planes.
- `Frustum3D`: six inward-facing planes extracted from a combined
  projection * view matrix, with inclusive point, AABB, and sphere tests.

This is a math/query foundation only. It is not Physics3D, a broadphase, a
collision solver, frustum culling, a renderer, or a device-runtime feature.

## Coordinate and numeric conventions

The geometry API follows the current StormEngine spatial convention:
`+X` right, `+Y` up, and `+Z` forward. Coordinates are represented as
single-precision `Vector3` values; intermediate geometric calculations use
double precision. A result that cannot be represented as a finite `float`
returns `InvalidArgument` instead of silently producing infinity.

AABB bounds are inclusive. Degenerate bounds (one or more zero-size axes) are
valid. Half-extents must be finite and non-negative. Sphere radii must be finite
and non-negative; a radius of zero represents one point.

A plane is represented by
`dot(normal, point) + offset = 0`. The factory normalizes the supplied normal
and computes a finite float offset. Signed distance is positive on the normal
side of the plane and negative on the opposite side.

A `Ray3D` can only be created from a finite origin and finite non-zero
direction. Its factory normalizes direction, so query distances and
`RayHit3D::distance` are world-space distances even when the supplied
direction has a different magnitude.

A frustum is extracted from a column-vector `projection * view` matrix.
The clip volume is `-w <= x <= w`, `-w <= y <= w`, and `0 <= z <= w`;
its normalized plane normals point into the visible volume. Point containment,
AABB intersection (positive-vertex plane test), and sphere intersection use
inclusive boundaries, with a scale-aware tolerance based on float precision
to avoid rejecting points exactly on a plane after matrix/plane normalization.
Invalid or degenerate matrices are rejected by the factory. These are culling
queries only; no renderer consumes them yet.

## Ray-query contract

All intersections require a finite, non-negative `max_distance`; the limit is
inclusive. Invalid rays, shapes, planes, or limits return a typed
`InvalidArgument` error. A valid query with no intersection returns an empty
`std::optional`. A hit contains the world-space point, distance, and surface
normal.

For AABBs, the implementation uses a three-axis slab test and deterministic
axis ordering for tied entry distances. For spheres, it uses the normalized-ray
quadratic and reports tangent hits. For planes, parallel and coplanar rays
return no hit because there is no unique intersection point. Plane hit normals
are oriented against the incoming ray. When a ray starts strictly inside an
AABB or sphere, the first hit is distance zero and its normal is zero because
there is no entry surface at the ray origin.

The query functions do not allocate and have no platform or Core dependency.
The current primitives are value types and all query functions are
single-call calculations; they do not own GPU or physics resources.

## Validation

The dedicated `stormengine_geometry3d_tests` executable covers bounds,
inclusive overlap, overflow rejection, sphere containment, plane construction
and signed distance, ray normalization, AABB slab hits, parallel misses,
sphere tangent/inside cases, plane orientation, invalid query limits, frustum
extraction for right-handed zero-to-one perspective projection, point/AABB/sphere
classification, inclusive boundary handling, and rejection of degenerate matrices.

The Geometry3D module is compiled as part of `Storm::Math`. Host tests and
Android/iOS target compilation are validated by the repository CI. A successful
cross-compile does not constitute runtime or rendering verification on a
physical device.

## Not implemented here

Oriented boxes, rays against meshes, swept-volume queries, collision
manifolds, 3D physics, camera culling integration, GPU resources, and device
rendering remain separate future work.
