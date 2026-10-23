# ADR 0005: SceneRenderer2D bridge to the CPU reference canvas

- **Status:** Accepted for the initial scene-to-pixels vertical slice
- **Date:** 2026-10-23
- **Scope:** 2D scene traversal and CPU reference rasterization; not a material ABI or production RHI

## Context

StormEngine has an in-memory Scene/Entity model, validated dimension-aware transforms, a tested orthographic Camera2D, and a bounded CPU canvas, but those parts were not connected. The first scene-to-pixels path must be testable without a window, GPU, or new runtime dependencies.

## Decision

1. Add SceneRenderer2D::render as a narrow render-layer adapter. It accepts a Dimension::Two scene, a valid Camera2D, a non-empty SoftwareCanvas2D, and requires camera viewport dimensions to match the canvas exactly.
2. Before clearing the framebuffer, traverse entities and verify that each Transform::to_matrix() succeeds, the composed camera/model matrix is finite, and all four local unit-quad vertices transform to representable points. Invalid configuration or unrepresentable transforms leave the previous framebuffer unchanged.
3. Compose transforms using the documented column-vector convention: camera.world_to_pixel_matrix() * entity.transform().to_matrix(). Visit entities through the scene's stable-ID-ordered non-allocating visitor.
4. Until renderable components or material/mesh resources are designed, every 2D entity is represented by a unit local quad. Transform2D controls its position, rotation and scale. Callers provide background/entity colors; there is no implication that the scene currently owns visual assets.
5. Return Unsupported for 1D/3D scenes instead of silently flattening them into 2D. Keep the implementation in the core library without adding a windowing or graphics dependency.

## Consequences and deferred work

- Headless tests prove the actual scene transforms reach rendered pixels and invalid transformations do not clear a previously rendered frame.
- The optional SDL sample now creates/updates a real Scene and delegates drawing to SceneRenderer2D; SDL remains the window/input/framebuffer presentation adapter.
- Renderable components, sprite/material assets, sorting/layers, batching, world hierarchy, interpolated transforms, GPU resource lifetime, shader handling, and production RHI remain future work.
- The unit-quad placeholder is intentionally temporary and must be replaced by an explicit visual-component contract rather than expanded into an implicit asset system.

## References

- ADR 0002: Coordinate system and matrix conventions.
- ADR 0003: Bounded CPU reference canvas.
- ADR 0004: Orthographic Camera2D coordinate mapping.
