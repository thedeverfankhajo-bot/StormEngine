# 2D Sprite Submission Layer

## Scope

`Storm::Rendering2D` is a platform-neutral CPU submission slice. It turns
Scene-owned `Sprite2DComponent` instances and their complete Transform2D
ancestry into bounded indexed quad geometry.

Every sprite validates a generation-bearing `TextureKey`, positive finite
size, finite pivot, normalized UV rectangle, and finite affine world matrix.
The key is an opaque batching identifier, not a texture object or GPU handle.
Texture loading, GPU allocation, upload, and resource lookup are outside this
module.

## Scene collection

`SpriteSceneCollector2D::build(scene, batch)` walks live sprite components
in deterministic entity order, obtains each full world matrix from
`TransformSystem2D`, and appends it. A failed collection clears partial
commands and returns the original typed error. Entities with no sprite are
ignored; entities with no Transform2D use the existing identity-transform
behavior.

The camera-aware overload also accepts a validated `Math::Camera2D`. It composes
the camera view-projection matrix with each entity's complete world transform
and emits positions in normalized device coordinates (NDC, [-1, 1]). The
camera viewport is supplied in logical viewport units by the caller. Invalid
camera parameters clear the previous batch and return a typed error. The
existing overload continues to emit world-space positions.

`Camera2DComponent` stores a complete Camera2D value on a Scene entity.
`SpriteSceneCollector2D::build_active_camera` ignores disabled cameras and
selects the enabled component with the greatest priority; ties are resolved by
Scene entity iteration order. The component's camera position/rotation remain
authoritative and are not implicitly taken from the owner's Transform2D. A
Scene without an enabled camera returns `InvalidState` and exposes no partial
batch.

## Geometry and painter order

A sprite emits four vertices and six 32-bit indices. The local quad spans
`[-pivot * size, (1 - pivot) * size]`; the world matrix is applied before
output. UVs are normalized, use a top-left texture origin, and support atlas
sub-rectangles. Vertex color is RGBA8.

Painter order is deterministic:
`sorting_layer`, then `sorting_order`, then original submission order.
Sprites are never globally reordered by texture. Only adjacent sprites with
the same texture key and sorting values coalesce into one
`SpriteBatchRange`, preserving alpha-blended order.

## Bounds and ownership

Default capacity is 4096 sprites; the supported maximum is 65,536. Pending
commands, vertices, indices, and ranges reserve capacity at creation.
Appending within capacity does not grow those vectors. Exhaustion returns a
typed `ResourceExhausted` error.

The batch is single-thread-affine and owns no Scene or GPU objects.
`SpriteBatchView` contains non-owning spans invalidated by append, clear,
build, move, or destruction.

## Current boundary

This is CPU scene collection and draw-geometry construction, not a GPU
renderer. It does not create a graphics instance/device, swapchain or
`CAMetalLayer` drawable, shader/pipeline, GPU vertex/index buffer, texture,
render pass, synchronization primitive, draw call, or presented frame.
iOS Metal and an Android graphics backend must be connected to the native
surface lifecycle before this phase can be called a rendering vertical slice.

## Validation

`stormengine_sprite_batch_tests` contains twelve deterministic host cases.
The CMake target is also included in Android arm64-v8a and iOS device arm64
cross-compilation. Cross-compilation is not physical-device rendering
validation.
