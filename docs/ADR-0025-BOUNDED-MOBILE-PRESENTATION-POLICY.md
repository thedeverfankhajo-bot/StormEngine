# ADR-0025: Bounded mobile presentation policy

- Status: Implemented as a backend-neutral policy and integrated with Android Vulkan; target CI and physical-device validation pending
- Date: 2026-11-20
- Scope: Presentation extent negotiation and bounded swapchain image-count selection

## Context

The current Android Vulkan smoke renderer creates a real clear-only frame, but its surface extent and image-count decisions were embedded in the native backend. Driver and surface capability values must be treated as inputs that require validation, and host tests should cover policy decisions without requiring the Android SDK.

## Decision

- Add a backend-neutral presentation policy under `Storm::Rendering`.
- A fixed platform extent is accepted only when it lies within the advertised minimum/maximum bounds.
- A variable extent is clamped from the requested drawable size into those bounds.
- Swapchain image selection prefers one image beyond the surface minimum, respects a finite surface maximum, and never exceeds a configured hard limit.
- Android Vulkan uses a hard limit of eight swapchain images and rejects unreasonable physical-device, queue-family, extension, surface-format, and swapchain-image enumeration counts.
- Android Vulkan rejects surfaces that cannot be used as color attachments or expose no supported 8-bit RGBA/BGRA format or composite-alpha mode.
- A single `VK_FORMAT_UNDEFINED` surface-format result is handled by selecting a supported 8-bit format while preserving the advertised color space.
- The acquired swapchain image index is validated before indexing framebuffers, and `VK_SUBOPTIMAL_KHR` during acquisition triggers recreation after presentation.

## Validation

The policy has six deterministic host test cases covering variable/fixed extents, invalid bounds, preferred image count, surface maximum, hard-limit behavior, and impossible capacities. Android NDK cross-compilation verifies integration with the Vulkan headers and NativeActivity target. Physical-device presentation remains a separate validation requirement.

## Deliberate limitations

This decision does not implement a reusable GraphicsDevice, command abstraction, shader compiler, graphics pipeline, vertex/index buffer uploads, texture resources, Scene-driven 2D/3D rendering, or Vulkan device-loss recovery. The existing Android Vulkan and iOS Metal paths remain clear-only presentation smoke tests.
