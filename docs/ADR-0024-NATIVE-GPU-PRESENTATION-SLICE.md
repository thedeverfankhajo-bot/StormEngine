# ADR-0024: First native GPU presentation slice

- Status: Implemented; cross-compilation validation in progress
- Date: 2026-11-20
- Scope: Android Vulkan and iOS Metal clear-only presentation

## Decision

- Android NativeActivity owns a Vulkan presentation helper for the lifetime of its ANativeWindow.
- The helper creates a Vulkan instance, Android surface, graphics/present queue, swapchain, image views, clear render pass, framebuffers, command pool/buffer, semaphores, and a frame fence.
- A failed swapchain rebuild releases every partially created swapchain-owned object before returning the error, keeping the helper from exposing stale partial resources.
- NativeActivity polls at a bounded 16 ms interval only while the lifecycle permits game-loop execution, and submits a clear-only frame. It tears down GPU objects before the native window is detached and recreates swapchain resources after resize.
- Android minimum SDK is 24 because the Vulkan loader is part of the Android platform from API 24. Devices below that level need a separately designed OpenGL ES fallback; they are not silently treated as Vulkan-capable.
- The iOS host view uses CAMetalLayer, MTLDevice, MTLCommandQueue, CADisplayLink, a render-pass descriptor, and a command buffer that clears and presents a drawable while the app is active.

## Deliberate limitations

- This is a real GPU clear-and-present smoke slice, not a SpriteBatch renderer.
- No shader pipeline, vertex/index buffers, texture upload, scene extraction, depth buffer, or 3D draw submission is included.
- Android Vulkan device-loss recovery is not yet implemented. Swapchain recreation handles out-of-date/suboptimal presentation results and surface resize.
- iOS Metal device availability is checked; no software renderer fallback is claimed.
- Physical Android/iOS device verification remains required.

## Validation

- Host CMake tests do not compile platform SDK-specific GPU translation units.
- Android NDK and iOS Device SDK jobs compile the native backends.
- A successful cross-compile does not prove runtime presentation on hardware.
