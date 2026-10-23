# ADR 0003: Bounded CPU reference canvas for the first rendering slice

- **Status:** Accepted for the first deterministic 2D rendering path
- **Date:** 2026-10-23
- **Scope:** reference rasterizer and bootstrap sample, not the final renderer backend

## Context

The initial desktop sample draws a rectangle with SDL's convenience renderer. That proves a window can display pixels but does not exercise an engine-owned draw path, and makes rendering behavior difficult to test in headless CI.

## Decision

Implement a dependency-free `SoftwareCanvas2D` in StormEngine itself. It owns a row-major RGBA8 framebuffer, transforms rectangle corners through the documented column-vector `Matrix4` convention, rasterizes the complete convex quad using pixel-center samples, clips iteration bounds before integer conversion, and composites straight-alpha colors over the destination.

The framebuffer is bounded to 8,192 pixels per axis and at most 16,777,216 pixels (64 MiB). Resize is failure-atomic. Zero-sized surfaces are allowed for minimized windows. Invalid/non-finite transforms and negative sizes return explicit errors without drawing. The implementation is single-owner-thread only and is not claimed to be production optimized.

The SDL3 example becomes a thin platform/presentation adapter: StormEngine creates the pixels, while SDL handles window/input events and presents the framebuffer from a streaming texture. The ordinary core build has no SDL dependency; tests need neither a GPU nor a window system.

## Consequences and deferred work

- The test suite can verify fill, transformed geometry, clipping, alpha compositing, memory limits and failure atomicity.
- The canvas can be used as a correctness oracle for future optimized CPU/GPU paths.
- Camera mapping is now defined by [ADR 0004](0004-camera2d-coordinate-mapping.md). Batching, texture/sprite assets, GPU resource lifetime, shader handling and a production Render Hardware Interface remain future work.
- This ADR does not choose Vulkan, Direct3D, Metal, OpenGL or a commercial renderer backend.
