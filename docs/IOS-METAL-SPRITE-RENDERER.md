# iOS Metal SpriteBatch Host Integration

## Scope

The iOS host application contains a deliberately small adapter from the
platform-neutral Scene/Rendering2D data model to Metal. It is an executable
integration slice, not a general-purpose graphics API abstraction.

## Data flow

1. The host controller creates a Scene with a Camera2DComponent and two
   Sprite2DComponent entities. Their Transform2D values are stored through
   TransformSystem2D.
2. Each CADisplayLink callback asks
   SpriteSceneCollector2D::build_active_camera to collect visible sprite
   geometry and apply the active camera view-projection transform.
3. The host validates the fixed demo capacity and the returned range bounds.
   The current demo binds TextureKey { id=1, generation=1 } to a 1×1
   RGBA8 white texture; vertex tint supplies the two sprite colors.
4. CPU SpriteVertex values are converted to a 32-byte Metal vertex layout
   (float2 position, float2 UV, float4 color). Vertex and 32-bit index data
   are copied into one of three shared-storage Metal buffer pairs.
5. A runtime-compiled MSL vertex/fragment program samples the texture,
   multiplies by vertex tint, and uses source-alpha blending. The host issues
   one indexed triangle draw for each contiguous SpriteBatchRange.
6. The command buffer presents the CAMetalLayer drawable. A three-slot
   semaphore prevents a frame slot from being reused before its command
   buffer completes.

The camera viewport is updated from the view's logical point dimensions when
UIKit lays out the host view. The drawable size continues to use the view's
pixel scale.

## Resource lifetime and limits

The demo SpriteBatch is capped at eight sprites. The host allocates three
vertex buffers and three index buffers sized to that cap, plus one texture,
one sampler, one pipeline state, one command queue, and one frame semaphore.
A missing GPU resource prevents frame submission. Unsupported texture keys or
out-of-range index spans are rejected before presentation. The display link
is stopped before teardown; outstanding frame slots are drained before the
Scene, SpriteBatch, and Metal resources are released.

## Current limitations

- Only the iOS host application uses this Scene-to-GPU adapter.
- Android Vulkan remains clear-only; it does not consume SpriteBatch data.
- The demo texture is a single white pixel. Arbitrary TextureKey lookup,
  image decoding, texture upload queues, atlases, and sampler caching are not
  connected.
- GPU resources are owned directly by the host controller. There is no public
  GraphicsDevice, GraphicsContext, resource handle registry, or reusable
  cross-platform backend contract.
- The MSL source is embedded and compiled at runtime. There is no shader
  asset pipeline, offline compiler, shader cache, or shared shader IR.
- RenderGraph remains a backend-independent scheduling/lifetime plan; no
  Metal/Vulkan executor or resource-barrier translation exists.
- No renderer performance counters, GPU timestamp queries, or 3D pipeline
  are included in this slice.

## Validation status

The CPU geometry and Scene collection contract is covered by the existing
host-side stormengine_sprite_batch_tests. The iOS app target is configured to
link Storm::Rendering2D and is compiled by the repository's iOS Device SDK CI
job when that workflow runs for the relevant commit.

A target build proves compilation/linking only. It does not execute the MSL
program or prove Metal device/simulator behavior. This slice must remain
unverified until an iOS Simulator run succeeds; physical-device verification
is a separate status and requires a real iPhone or iPad.