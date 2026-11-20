# iOS Metal Pipeline Smoke Slice

## Scope

The iOS host view now exercises the minimum executable Metal draw path instead of only clearing the drawable:

1. It obtains the system Metal device and creates a command queue.
2. It compiles a small embedded Metal Shading Language program containing one vertex and one fragment entry point.
3. It creates a render-pipeline state using the active `CAMetalLayer` pixel format.
4. For each active `CADisplayLink` callback, it acquires a drawable, creates a render pass and command encoder, binds the pipeline and three interleaved position/color vertices, sets the drawable-sized viewport, and submits one triangle primitive.
5. It presents the drawable and commits the command buffer.

The vertex record is explicitly 16-byte aligned and stores two `float4` values, matching the Metal shader's layout. The three vertices are supplied with `setVertexBytes`; this smoke path does not create a persistent vertex buffer or GPU resource cache. If pipeline compilation or creation fails, the host continues to submit the clear-only fallback frame and logs the underlying Metal error.

## Ownership and failure behavior

The view controller owns the retained command queue and pipeline state under the project's manual Objective-C reference-counting configuration. Temporary library, function, and descriptor objects are released after pipeline creation. The pipeline state is released before the Metal device during controller teardown. A missing device, queue, shader entry point, drawable, command buffer, or encoder does not cause an invalid draw command to be submitted.

Frame submission is limited to an active UIApplication and driven by CADisplayLink. The existing surface lifecycle remains responsible for drawable-size metadata; the rendering code reads the actual drawable texture extent when setting its viewport.

## Explicit limitations

This is an iOS Metal backend smoke draw, not the engine's renderer. It does not:
- consume Scene, Camera2D, or SpriteBatch data;
- upload or sample textures;
- expose a backend-neutral GraphicsDevice/GraphicsContext;
- manage persistent vertex/index buffers or resource lifetimes;
- execute RenderGraph passes;
- implement depth, blending, materials, lighting, or 3D meshes;
- replace the Android Vulkan clear-only path.

The next rendering slice must factor the native draw path behind backend-neutral engine contracts and submit validated Scene/SpriteBatch geometry, while keeping Metal and Vulkan API types out of Foundation/Core public headers.

## Validation status

Source implementation: present in `platforms/ios/app/app_delegate.mm`.

Automated host validation: no host CTest exercises Metal commands. The iOS target must be cross-compiled to validate Objective-C++ and Metal API integration; simulator/device execution and physical-device presentation remain separate validation steps. This change is not Device-Verified.
