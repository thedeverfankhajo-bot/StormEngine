# ADR 0004: Orthographic Camera2D coordinate mapping

- **Status:** Accepted for the first deterministic 2D rendering slice
- **Date:** 2026-10-23
- **Scope:** world-to-framebuffer mapping for the CPU reference renderer; not a GPU projection ABI

## Context

The CPU reference canvas can rasterize transformed rectangles, but the caller must currently invent a mapping from world coordinates to pixels. The mapping must match the engine's world convention and remain independently testable without SDL, a GPU, or a backend-specific projection API.

## Decision

1. `Camera2D` is an allocation-free value in `storm::render`. It stores a world-space center, counter-clockwise camera rotation in radians, visible world-space height at zoom 1, positive zoom, and unsigned viewport pixel dimensions.
2. World coordinates use +X right and +Y up. Pixel coordinates use a continuous origin at the viewport's top-left, with +X right and +Y down. The camera center maps to half the viewport width and height.
3. The pixel scale is `viewport_height * zoom / view_height`, so the vertical world span defines scale and the viewport aspect ratio determines the horizontal span. Zoom above 1 moves the camera closer. The view transform rotates world displacement by the negative camera angle, then flips the Y axis for pixels.
4. `world_to_pixel_matrix()` returns a column-vector `Matrix4` suitable for composition with local object transforms. Point-conversion methods return no value for invalid input/configuration or results that cannot be represented as finite floats. A zero viewport is invalid, including during minimization.
5. Camera mapping does not round or clip coordinates; rasterization and viewport clipping remain the canvas's responsibility. The API performs no allocation, I/O, platform calls, or synchronization.
6. `set_zoom_at_pixel()` changes zoom while shifting the camera center so the world point beneath the supplied pixel remains stable. Invalid requests or an unrepresentable adjusted center fail without partially changing the camera; application/editor code owns its permitted zoom range.

## Consequences and deferred work

- World-space movement and mouse-pixel conversion can be tested deterministically and used in the headless reference path.
- The SDL sample demonstrates world-space movement, picking conversion, and camera zoom while keeping SDL as a window/input/texture-present adapter.
- Camera hierarchy, parent/world transforms, camera bounds, pixel-perfect policies, multiple viewports, GPU projection conventions, a 3D camera and production render-device resources remain future work.
