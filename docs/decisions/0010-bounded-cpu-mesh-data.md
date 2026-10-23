# ADR 0010: Bounded CPU MeshData triangle lists

- **Status:** Accepted as a 3D geometry data primitive
- **Date:** 2026-10-23
- **Scope:** CPU-side geometry validation and ownership; not a GPU resource ABI or renderer decision

## Context

StormEngine has foundational 3D vectors, transforms, quaternions and explicit perspective-projection math, but it lacks a bounded, engine-owned representation for indexed triangle geometry. Adding GPU allocations or a graphics backend before this data contract exists would mix validation and device lifetime too early.

## Decision

1. Add render::MeshVertex with local-space position, normal and UV coordinates, and render::MeshData that owns vertex and 32-bit index arrays.
2. Accept only indexed triangle lists: at least one vertex, at least three indices, and an index count divisible by three. Each index must refer to an existing vertex.
3. Reject non-finite position, normal or UV attributes. Do not silently normalize normals, change winding, clamp UVs, generate attributes, or flip a UV axis.
4. Bound each mesh to 1,000,000 vertices and 3,000,000 indices. Return an explicit status for invalid data, resource-budget violations and allocation failure.
5. Replace geometry transactionally: all validation and allocations complete before the existing arrays are swapped. Failed updates preserve the current mesh; reset() explicitly releases data.
6. Keep the implementation independent of graphics APIs and third-party libraries. It does not upload buffers, decode model files, create materials/submeshes, calculate bounds, serialize geometry or promise a rendering performance level.

## Validation

A dedicated headless test executable covers valid triangle storage, finite attribute checks, malformed index lists, out-of-range indices, failure atomicity, reset behavior and the policy of preserving finite zero normals.

## Consequences

- Model importers and future RHI backends can target one tested, bounded CPU geometry contract.
- Mesh file parsing, vertex-layout versioning, bounds generation, index optimization, meshlets, GPU buffer lifetime and cross-backend winding conventions remain separate decisions.
- The maximum counts bound input size before copying; they do not guarantee a particular allocator will successfully allocate the requested storage.

## References

- Architecture and roadmap.
- ADR 0002: Coordinate system and matrix conventions.
- ADR 0009: Explicit perspective projection math.
