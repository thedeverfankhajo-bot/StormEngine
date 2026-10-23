# ADR 0006: Explicit Renderable2D data and scene format v2 migration

- **Status:** Accepted for the early CPU reference renderer
- **Date:** 2026-10-23
- **Scope:** persisted 2D quad metadata and forward-readable scene persistence; not a general ECS or material ABI

## Context

The initial SceneRenderer2D drew every 2D entity as a unit quad. This connected the camera, transforms and canvas, but made logic-only entities visible and offered no per-entity geometry size, tint or visibility. Persisting the new visual state also requires a migration policy because version 1 files have no component section.

## Decision

1. Add optional `scene::Renderable2D` data to Entity. It stores a finite positive local size, four straight-RGBA tint bytes and a visibility flag. New entities have no renderable component, so logic-only entities remain invisible.
2. Attach the component only to 2D entities. Reject zero, negative or non-finite sizes and preserve the old component if an update is invalid.
3. SceneCodec writes version 2. Each record contains a little-endian component mask after its dimension-specific transform; bit 0 indicates the fixed 13-byte Renderable2D payload. Unknown bits return `Unsupported`; malformed sizes/visibility and use in a non-2D scene fail transactionally.
4. Continue reading version 1. Since version 1 has no renderable state, migrate each 2D entity to the default visible 1×1 white-tinted quad, preserving the former reference-renderer behavior. Migrated scenes are written as version 2 when saved. 1D/3D entities never gain a Renderable2D component.
5. SceneRenderer2D only draws entities with a visible Renderable2D. Its call-site entity color is the base color and the component tint multiplies all four channels using rounded integer arithmetic. The renderer preflights visible geometry before clearing the previous framebuffer.

## Consequences

- The scene format is explicitly versioned, bounded to 64 MiB, deterministic, and strict about unknown required component bits.
- Unit tests cover version-2 component round-trip, malformed component masks, and a version-1 migration fixture, in addition to existing malformed input and transactionality cases.
- This is one concrete scene-data component, not a general ECS. Texture/sprite assets, materials, blend/sampler state, render layers, GPU resource lifetimes, batching, hierarchy, and production RHI remain separate future architecture decisions.
- The initial CPU rasterizer remains a correctness baseline, not a performance claim or a 3D backend.

## References

- ADR 0002: Coordinate system and matrix conventions.
- ADR 0003: Bounded CPU reference canvas.
- ADR 0005: SceneRenderer2D bridge to the CPU reference canvas.
- [Scene file format](../SCENE_FORMAT.md)
