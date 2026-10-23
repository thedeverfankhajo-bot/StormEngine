# ADR 0007: Bounded CPU Texture2D data and reference-canvas sampling

- **Status:** Accepted as the first texture-data primitive
- **Date:** 2026-10-23
- **Scope:** CPU RGBA8 image ownership and deterministic nearest-neighbor sampling; not a file importer or GPU texture abstraction

## Context

The reference canvas and explicit Renderable2D contract can draw tinted geometry, but no texture data can be owned or sampled. Adding an external image decoder now would add a dependency, image-format and license surface before persistent asset ownership exists.

## Decision

1. Add `render::Texture2D` as an owning CPU RGBA8 pixel buffer. Dimensions must be non-zero, at most 8,192 per axis, and within the shared 16,777,216-pixel (64 MiB) budget. Input span length must exactly match width × height.
2. Pixel replacement is failure-atomic. The existing image remains unchanged when dimensions, payload length or allocation is invalid; `reset()` explicitly empties it.
3. Add `SoftwareCanvas2D::draw_texture` with an affine 2D transform, positive/zero local size behavior matching `draw_rect`, nearest-neighbor sampling at pixel centers, per-channel RGBA tint, clipping and straight-alpha source-over blending.
4. Reject empty/malformed textures, negative size, non-finite transforms and perspective/non-2D transforms before drawing. A zero-alpha texel or tint leaves the destination unchanged.
5. Keep Texture2D independent of Scene persistence and GPU resource lifetimes. This makes it useful as a headless correctness reference without freezing a future asset handle, file-import or RHI ABI.

## Consequences

- Tests can verify ownership, transactionality, sample coordinates, tint, transparent pixels and invalid input without a window, GPU or third-party library.
- PNG/JPEG/etc. decoding, content hashes, stable asset IDs, project-root-confined loading, mipmaps, filtering modes, sampler/material state, batching and GPU residency remain future work.
- The current SDL3 sample still uses solid-color Renderable2D geometry; integrating persistent texture references is the next asset-contract step.
- The CPU path is a correctness baseline, not a performance claim for production use.

## References

- ADR 0003: Bounded CPU reference canvas.
- ADR 0005: SceneRenderer2D bridge to the CPU reference canvas.
- ADR 0006: Renderable2D and scene format v2 migration.
- [Architecture](../ARCHITECTURE.md)
- [Roadmap](../ROADMAP.md)
