#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <dispatch/dispatch.h>

#import "storm/ios/scene_lifecycle_delegate.h"

#include "storm/platform/surface_lifecycle.hpp"
#include "storm/input/touch_input.hpp"
#include "storm/core/transform_system_2d.hpp"
#include "storm/rendering/sprite_batch.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace {

constexpr Storm::Platform::NativeSurfaceId kIOSHostSurfaceId = 1U;

struct alignas(16) MetalSpriteVertex final {
    float position[2];
    float uv[2];
    float color[4];
};
static_assert(sizeof(MetalSpriteVertex) == 32U);
static_assert(offsetof(MetalSpriteVertex, position) == 0U);
static_assert(offsetof(MetalSpriteVertex, uv) == 8U);
static_assert(offsetof(MetalSpriteVertex, color) == 16U);

constexpr std::size_t kDemoSpriteCapacity = 8U;
constexpr std::size_t kMetalFramesInFlight = 3U;

struct MetalFrameSemaphoreReservation final {
    dispatch_semaphore_t semaphore{nullptr};
    bool active{true};

    explicit MetalFrameSemaphoreReservation(dispatch_semaphore_t value) noexcept
        : semaphore(value) {}

    MetalFrameSemaphoreReservation(const MetalFrameSemaphoreReservation&) = delete;
    MetalFrameSemaphoreReservation& operator=(
        const MetalFrameSemaphoreReservation&) = delete;

    ~MetalFrameSemaphoreReservation() {
        if (active && semaphore != nullptr) {
            dispatch_semaphore_signal(semaphore);
        }
    }

    void transfer_to_command_buffer() noexcept {
        active = false;
    }
};

[[nodiscard]] bool timestamp_to_nanoseconds(double seconds, std::uint64_t& result) {
    if (!std::isfinite(seconds) || seconds < 0.0) return false;
    const long double nanoseconds = static_cast<long double>(seconds) * 1000000000.0L;
    if (!std::isfinite(nanoseconds) ||
        nanoseconds > static_cast<long double>(std::numeric_limits<std::uint64_t>::max())) {
        return false;
    }
    result = static_cast<std::uint64_t>(nanoseconds);
    return true;
}

[[nodiscard]] Storm::Result<void> create_demo_scene(Storm::Scene& scene) {
    using namespace Storm;
    using namespace Storm::Math;
    using namespace Storm::Rendering;

    try {
    auto camera_entity = scene.create_entity("Main Camera");
    if (!camera_entity) {
        return Result<void>::failure(camera_entity.error());
    }

    Camera2D camera{
        .position = {0.0F, 0.0F},
        .rotation_radians = 0.0F,
        .viewport_size = {1.0F, 1.0F},
        .zoom = 1.0F,
    };
    auto camera_component = scene.add_component<Camera2DComponent>(
        camera_entity.value(),
        Camera2DComponent{.camera = camera, .priority = 100, .enabled = true});
    if (!camera_component) {
        return Result<void>::failure(camera_component.error());
    }

    auto left_entity = scene.create_entity("Coral Sprite");
    if (!left_entity) {
        return Result<void>::failure(left_entity.error());
    }
    Sprite2DComponent left_sprite{
        .texture = TextureKey{1U, 1U},
        .size = {92.0F, 92.0F},
        .pivot = {0.5F, 0.5F},
        .uv = {},
        .color = SpriteColor{255U, 104U, 82U, 255U},
        .sorting_layer = 0,
        .sorting_order = 0,
    };
    auto left_component = scene.add_component<Sprite2DComponent>(
        left_entity.value(), left_sprite);
    if (!left_component) {
        return Result<void>::failure(left_component.error());
    }
    const auto left_transform = TransformSystem2D::set_local_transform(
        scene, left_entity.value(),
        Transform2D{.translation = {-54.0F, 0.0F}});
    if (!left_transform) {
        return left_transform;
    }

    auto right_entity = scene.create_entity("Blue Sprite");
    if (!right_entity) {
        return Result<void>::failure(right_entity.error());
    }
    Sprite2DComponent right_sprite{
        .texture = TextureKey{1U, 1U},
        .size = {92.0F, 92.0F},
        .pivot = {0.5F, 0.5F},
        .uv = {},
        .color = SpriteColor{75U, 188U, 245U, 255U},
        .sorting_layer = 0,
        .sorting_order = 1,
    };
    auto right_component = scene.add_component<Sprite2DComponent>(
        right_entity.value(), right_sprite);
    if (!right_component) {
        return Result<void>::failure(right_component.error());
    }
    return TransformSystem2D::set_local_transform(
        scene, right_entity.value(),
        Transform2D{.translation = {54.0F, 0.0F}});
    } catch (const std::bad_alloc&) {
        return Storm::Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::ResourceExhausted,
            "Unable to allocate the bounded iOS Metal demo Scene"));
    } catch (const std::length_error&) {
        return Storm::Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::ResourceExhausted,
            "The iOS Metal demo Scene exceeded a container limit"));
    }
}

[[nodiscard]] Storm::Result<void> cancel_touch_input(
    Storm::Input::TouchInput& input,
    std::uint64_t timestamp_ns) {
    auto cancelled = input.cancel_all(timestamp_ns);
    if (!cancelled &&
        cancelled.error().code == Storm::ErrorCode::ResourceExhausted) {
        // Preserve all prior events unless capacity prevents reporting every
        // cancellation. In that case the cancellation state takes priority.
        input.begin_frame();
        cancelled = input.cancel_all(timestamp_ns);
    }
    return cancelled;
}

[[nodiscard]] Storm::Result<void> detach_touch_input(
    Storm::Input::TouchInput& input,
    std::uint64_t surface_generation,
    std::uint64_t timestamp_ns) {
    auto detached = input.detach_surface(surface_generation, timestamp_ns);
    if (!detached &&
        detached.error().code == Storm::ErrorCode::ResourceExhausted) {
        // Surface teardown must not strand pointers. Retry after discarding
        // older buffered moves only when the cancellation batch cannot fit.
        input.begin_frame();
        detached = input.detach_surface(surface_generation, timestamp_ns);
    }
    return detached;
}

} // namespace

@interface StormEngineMetalView : UIView
@end

@implementation StormEngineMetalView
+ (Class)layerClass { return [CAMetalLayer class]; }
@end

@interface StormEngineHostViewController : UIViewController {
@private
    void *_surfaceLifecycleStorage;
    void *_touchInputStorage;
    UILabel *_statusLabel;
    id<MTLDevice> _metalDevice;
    id<MTLCommandQueue> _metalQueue;
    id<MTLRenderPipelineState> _metalPipeline;
    id<MTLTexture> _spriteTexture;
    id<MTLSamplerState> _spriteSampler;
    id<MTLBuffer> _spriteVertexBuffers[3];
    id<MTLBuffer> _spriteIndexBuffers[3];
    dispatch_semaphore_t _inFlightSemaphore;
    NSUInteger _frameBufferIndex;
    void *_renderSceneStorage;
    void *_spriteBatchStorage;
    CADisplayLink *_displayLink;
}

- (void)_detachSurface;
- (void)_renderMetalFrame;
- (void)_cancelActiveTouches;
- (void)_reportSurfaceError:(const Storm::Error &)error;
- (void)_reportTouchError:(const Storm::Error &)error;
- (void)_processTouches:(NSSet<UITouch *> *)touches phase:(Storm::Input::PointerPhase)phase timestamp:(NSTimeInterval)timestamp;

@end

@implementation StormEngineHostViewController

- (void)loadView {
    StormEngineMetalView *metalView = [[StormEngineMetalView alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.view = metalView;
    [metalView release];
}

- (instancetype)init {
    self = [super initWithNibName:nil bundle:nil];
    if (self == nil) {
        return nil;
    }

    auto *surface = new (std::nothrow) Storm::Platform::SurfaceLifecycle();
    if (surface == nullptr) {
        [self release];
        return nil;
    }

    auto input_result = Storm::Input::TouchInput::create();
    if (!input_result) {
        delete surface;
        [self release];
        return nil;
    }
    auto *touch_input = new (std::nothrow)
        Storm::Input::TouchInput(std::move(input_result).value());
    if (touch_input == nullptr) {
        delete surface;
        [self release];
        return nil;
    }

    auto scene_result = Storm::Scene::create("StormEngine Metal Demo");
    if (!scene_result) {
        delete touch_input;
        delete surface;
        [self release];
        return nil;
    }
    auto *scene = new (std::nothrow) Storm::Scene(std::move(scene_result).value());
    if (scene == nullptr) {
        delete touch_input;
        delete surface;
        [self release];
        return nil;
    }
    const auto scene_setup = create_demo_scene(*scene);
    if (!scene_setup) {
        delete scene;
        delete touch_input;
        delete surface;
        [self release];
        return nil;
    }

    auto batch_result = Storm::Rendering::SpriteBatch::create(
        Storm::Rendering::SpriteBatchConfig{.maximum_sprites = kDemoSpriteCapacity});
    if (!batch_result) {
        delete scene;
        delete touch_input;
        delete surface;
        [self release];
        return nil;
    }
    auto *batch = new (std::nothrow)
        Storm::Rendering::SpriteBatch(std::move(batch_result).value());
    if (batch == nullptr) {
        delete scene;
        delete touch_input;
        delete surface;
        [self release];
        return nil;
    }

    _surfaceLifecycleStorage = surface;
    _touchInputStorage = touch_input;
    _renderSceneStorage = scene;
    _spriteBatchStorage = batch;
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor systemBackgroundColor];
    self.view.multipleTouchEnabled = YES;

    _metalDevice = MTLCreateSystemDefaultDevice();
    if (_metalDevice != nil) {
        _metalQueue = [_metalDevice newCommandQueue];
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.view.layer;
        metalLayer.device = _metalDevice;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = YES;
        metalLayer.contentsScale = self.view.contentScaleFactor;

        if (_metalQueue == nil) {
            NSLog(@"StormEngine: Metal command queue creation failed.");
        } else {
            NSString *shaderSource =
                @"#include <metal_stdlib>\n"
                 "using namespace metal;\n"
                 "struct VertexData { float2 position; float2 uv; float4 color; };\n"
                 "struct RasterData { float4 position [[position]]; float2 uv; float4 color; };\n"
                 "vertex RasterData storm_vertex(\n"
                 "    uint vertex_id [[vertex_id]],\n"
                 "    const device VertexData* vertices [[buffer(0)]]) {\n"
                 "    RasterData output;\n"
                 "    output.position = float4(vertices[vertex_id].position, 0.0, 1.0);\n"
                 "    output.uv = vertices[vertex_id].uv;\n"
                 "    output.color = vertices[vertex_id].color;\n"
                 "    return output;\n"
                 "}\n"
                 "fragment float4 storm_fragment(\n"
                 "    RasterData input [[stage_in]],\n"
                 "    texture2d<float> color_texture [[texture(0)]],\n"
                 "    sampler color_sampler [[sampler(0)]]) {\n"
                 "    return color_texture.sample(color_sampler, input.uv) * input.color;\n"
                 "}\n";

            NSError *libraryError = nil;
            id<MTLLibrary> library = [_metalDevice
                newLibraryWithSource:shaderSource options:nil error:&libraryError];
            if (library == nil) {
                NSLog(@"StormEngine: Metal shader compilation failed: %@", libraryError);
            } else {
                id<MTLFunction> vertexFunction = [library newFunctionWithName:@"storm_vertex"];
                id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"storm_fragment"];
                if (vertexFunction == nil || fragmentFunction == nil) {
                    NSLog(@"StormEngine: Metal shader library is missing an entry point.");
                } else {
                    MTLRenderPipelineDescriptor *descriptor =
                        [[MTLRenderPipelineDescriptor alloc] init];
                    descriptor.vertexFunction = vertexFunction;
                    descriptor.fragmentFunction = fragmentFunction;
                    MTLRenderPipelineColorAttachmentDescriptor *colorAttachment =
                        descriptor.colorAttachments[0];
                    colorAttachment.pixelFormat = metalLayer.pixelFormat;
                    colorAttachment.blendingEnabled = YES;
                    colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
                    colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;
                    colorAttachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                    colorAttachment.destinationRGBBlendFactor =
                        MTLBlendFactorOneMinusSourceAlpha;
                    colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
                    colorAttachment.destinationAlphaBlendFactor =
                        MTLBlendFactorOneMinusSourceAlpha;

                    NSError *pipelineError = nil;
                    _metalPipeline = [_metalDevice
                        newRenderPipelineStateWithDescriptor:descriptor error:&pipelineError];
                    if (_metalPipeline == nil) {
                        NSLog(@"StormEngine: Metal render-pipeline creation failed: %@",
                              pipelineError);
                    }
                    [descriptor release];
                }
                [vertexFunction release];
                [fragmentFunction release];
                [library release];
            }

            if (_metalPipeline != nil) {
                MTLTextureDescriptor *textureDescriptor =
                    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
                        MTLPixelFormatRGBA8Unorm width:1 height:1 mipmapped:NO];
                textureDescriptor.usage = MTLTextureUsageShaderRead;
                _spriteTexture = [_metalDevice newTextureWithDescriptor:textureDescriptor];
                if (_spriteTexture != nil) {
                    const std::uint32_t whitePixel = 0xffffffffU;
                    [_spriteTexture replaceRegion:MTLRegionMake2D(0U, 0U, 1U, 1U)
                                      mipmapLevel:0U
                                        withBytes:&whitePixel
                                      bytesPerRow:sizeof(whitePixel)];
                }

                MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
                samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
                samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
                samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
                samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
                _spriteSampler = [_metalDevice newSamplerStateWithDescriptor:samplerDescriptor];
                [samplerDescriptor release];

                const NSUInteger vertexBytes = static_cast<NSUInteger>(
                    sizeof(MetalSpriteVertex) * 4U * kDemoSpriteCapacity);
                const NSUInteger indexBytes = static_cast<NSUInteger>(
                    sizeof(std::uint32_t) * 6U * kDemoSpriteCapacity);
                for (std::size_t index = 0U; index < kMetalFramesInFlight; ++index) {
                    _spriteVertexBuffers[index] = [_metalDevice
                        newBufferWithLength:vertexBytes options:MTLResourceStorageModeShared];
                    _spriteIndexBuffers[index] = [_metalDevice
                        newBufferWithLength:indexBytes options:MTLResourceStorageModeShared];
                }
                _inFlightSemaphore = dispatch_semaphore_create(
                    static_cast<long>(kMetalFramesInFlight));
                if (_spriteTexture == nil || _spriteSampler == nil ||
                    _inFlightSemaphore == nil) {
                    NSLog(@"StormEngine: Metal sprite texture, sampler, or frame semaphore creation failed.");
                }
                for (std::size_t index = 0U; index < kMetalFramesInFlight; ++index) {
                    if (_spriteVertexBuffers[index] == nil ||
                        _spriteIndexBuffers[index] == nil) {
                        NSLog(@"StormEngine: Metal sprite buffer allocation failed.");
                    }
                }
            }
        }
    } else {
        NSLog(@"StormEngine: Metal is unavailable on this device.");
    }

    _statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(0.0, 0.0, 0.0, 0.0)];
    _statusLabel.numberOfLines = 0;
    _statusLabel.textAlignment = NSTextAlignmentCenter;
    _statusLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    _statusLabel.text = @"StormEngine\n\nWaiting for a valid iOS view surface...";
    _statusLabel.accessibilityIdentifier = @"stormengine.host.status";
    _statusLabel.translatesAutoresizingMaskIntoConstraints = NO;

    [self.view addSubview:_statusLabel];
    [NSLayoutConstraint activateConstraints:@[
        [_statusLabel.leadingAnchor
            constraintGreaterThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor
                                         constant:24.0],
        [_statusLabel.trailingAnchor
            constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor
                                      constant:-24.0],
        [_statusLabel.centerXAnchor
            constraintEqualToAnchor:self.view.safeAreaLayoutGuide.centerXAnchor],
        [_statusLabel.topAnchor
            constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor
                              constant:20.0]
    ]];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];

    Storm::Platform::SurfaceLifecycle *surface =
        static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    if (surface == nullptr) {
        return;
    }

    const CGRect bounds = self.view.bounds;
    if (_metalDevice != nil) {
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.view.layer;
        metalLayer.contentsScale = self.view.contentScaleFactor;
        metalLayer.drawableSize = CGSizeMake(
            bounds.size.width * self.view.contentScaleFactor,
            bounds.size.height * self.view.contentScaleFactor);
    }
    const double logicalWidth = static_cast<double>(bounds.size.width);
    const double logicalHeight = static_cast<double>(bounds.size.height);
    const double pixelScale = static_cast<double>(self.view.contentScaleFactor);

    if (!std::isfinite(logicalWidth) ||
        !std::isfinite(logicalHeight) ||
        logicalWidth <= 0.0 ||
        logicalHeight <= 0.0) {
        [self _detachSurface];
        return;
    }

    const Storm::Result<Storm::Platform::SurfaceExtent> extent =
        Storm::Platform::surface_extent_from_logical_size(
            logicalWidth, logicalHeight, pixelScale);
    if (!extent) {
        [self _detachSurface];
        [self _reportSurfaceError:extent.error()];
        return;
    }

    const auto current = surface->current();
    const Storm::Result<Storm::Platform::SurfaceDescriptor> updated =
        current.has_value()
            ? surface->resize(
                  current->native_id,
                  current->generation,
                  extent.value(),
                  pixelScale)
            : surface->attach(kIOSHostSurfaceId, extent.value(), pixelScale);

    if (!updated) {
        [self _detachSurface];
        [self _reportSurfaceError:updated.error()];
        return;
    }

    const Storm::Platform::SurfaceDescriptor &descriptor = updated.value();
    auto *render_scene = static_cast<Storm::Scene *>(_renderSceneStorage);
    if (render_scene != nullptr) {
        const float viewport_width = static_cast<float>(logicalWidth);
        const float viewport_height = static_cast<float>(logicalHeight);
        const auto cameras_updated =
            render_scene->for_each_component<Storm::Rendering::Camera2DComponent>(
                [viewport_width, viewport_height](
                    Storm::EntityHandle, Storm::Rendering::Camera2DComponent& component) {
                    component.camera.viewport_size = {viewport_width, viewport_height};
                });
        if (!cameras_updated) {
            NSLog(@"StormEngine: Could not update the Scene camera viewport: %s",
                  cameras_updated.error().message.c_str());
            return;
        }
    }
    if (!current.has_value()) {
        auto *touch_input = static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
        if (touch_input == nullptr) {
            [self _reportSurfaceError:Storm::Error::make(
                Storm::ErrorCode::InvalidState, "The iOS touch input adapter is unavailable.")];
            [self _detachSurface];
            return;
        }
        const auto input_attached = touch_input->attach_surface(descriptor.generation);
        if (!input_attached) {
            [self _reportTouchError:input_attached.error()];
            static_cast<void>(surface->detach(descriptor.native_id, descriptor.generation));
            return;
        }
    }
    BOOL sprite_renderer_ready =
        _metalQueue != nil && _metalPipeline != nil && _spriteTexture != nil &&
        _spriteSampler != nil && _inFlightSemaphore != nil;
    for (std::size_t index = 0U; index < kMetalFramesInFlight; ++index) {
        if (_spriteVertexBuffers[index] == nil || _spriteIndexBuffers[index] == nil) {
            sprite_renderer_ready = NO;
        }
    }
    _statusLabel.text = [NSString stringWithFormat:
        @"StormEngine\n\nNative iOS view surface attached\n"
        "%u × %u pixels · %.2f pixels per point\n"
        "Touch input bridge active.\n"
        "%@.",
        static_cast<unsigned int>(descriptor.extent.width),
        static_cast<unsigned int>(descriptor.extent.height),
        descriptor.pixels_per_logical_unit,
        (_metalQueue == nil
            ? @"Metal unavailable; presentation disabled"
            : (sprite_renderer_ready
                ? @"Metal Scene/SpriteBatch GPU indexed draw path ready"
                : @"Metal sprite renderer resources unavailable"))];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesBegan:touches withEvent:event];
    [self _processTouches:touches phase:Storm::Input::PointerPhase::Down timestamp:event.timestamp];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesMoved:touches withEvent:event];
    [self _processTouches:touches phase:Storm::Input::PointerPhase::Move timestamp:event.timestamp];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesEnded:touches withEvent:event];
    [self _processTouches:touches phase:Storm::Input::PointerPhase::Up timestamp:event.timestamp];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesCancelled:touches withEvent:event];
    [self _processTouches:touches phase:Storm::Input::PointerPhase::Cancel timestamp:event.timestamp];
}

- (void)_processTouches:(NSSet<UITouch *> *)touches phase:(Storm::Input::PointerPhase)phase timestamp:(NSTimeInterval)timestamp {
    if (![NSThread isMainThread]) return;
    auto *surface = static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    auto *touch_input = static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
    if (surface == nullptr || touch_input == nullptr) return;
    const auto current = surface->current();
    if (!current.has_value() || touch_input->surface_generation() !=
            std::optional<std::uint64_t>{current->generation}) return;

    const NSUInteger count = touches.count;
    if (count == 0U) return;
    if (count > Storm::Input::TouchInput::maximum_supported_pointers) {
        std::uint64_t cleanup_timestamp = 0U;
        if (timestamp_to_nanoseconds([[NSProcessInfo processInfo] systemUptime], cleanup_timestamp)) {
            const auto cancelled = cancel_touch_input(*touch_input, cleanup_timestamp);
            if (!cancelled) [self _reportTouchError:cancelled.error()];
        }
        return;
    }
    std::array<Storm::Input::PointerEvent,
               Storm::Input::TouchInput::maximum_supported_pointers> events{};
    std::size_t event_count = 0U;
    for (UITouch *touch in touches) {
        const CGPoint point = [touch locationInView:self.view];
        std::uint64_t timestamp_ns = 0U;
        if (!timestamp_to_nanoseconds(static_cast<double>(timestamp), timestamp_ns)) {
            [self _reportTouchError:Storm::Error::make(
                Storm::ErrorCode::InvalidArgument, "UIKit supplied an invalid event timestamp.")];
            std::uint64_t cleanup_timestamp = 0U;
            if (timestamp_to_nanoseconds([[NSProcessInfo processInfo] systemUptime], cleanup_timestamp)) {
                const auto cancelled = cancel_touch_input(*touch_input, cleanup_timestamp);
                if (!cancelled) [self _reportTouchError:cancelled.error()];
            }
            return;
        }
        const auto pointer_id = static_cast<Storm::Input::PointerId>(
            reinterpret_cast<std::uintptr_t>(touch));
        events[event_count++] = Storm::Input::PointerEvent{
            .pointer_id = pointer_id,
            .phase = phase,
            .x = static_cast<double>(point.x),
            .y = static_cast<double>(point.y),
            .timestamp_ns = timestamp_ns,
            .surface_generation = current->generation,
        };
    }
    std::sort(events.begin(), events.begin() + static_cast<std::ptrdiff_t>(event_count),
        [](const auto& first, const auto& second) {
            if (first.timestamp_ns != second.timestamp_ns)
                return first.timestamp_ns < second.timestamp_ns;
            return first.pointer_id < second.pointer_id;
        });

    // Keep separate UIKit callbacks in the same pending display frame.
    // The GameLoop adapter owns frame-buffer clearing after consumption.
    const auto processed = touch_input->process_batch(
        std::span<const Storm::Input::PointerEvent>{events.data(), event_count});
    if (!processed) [self _reportTouchError:processed.error()];
    if (_statusLabel != nil) {
        _statusLabel.text = [NSString stringWithFormat:
            @"StormEngine\n\nNative iOS touch input active · %lu pointers\n\n"
            "%@ · gameplay runtime is not integrated yet.",
            static_cast<unsigned long>(touch_input->active_pointer_count()),
            (_metalPipeline != nil
                ? @"Metal Scene/SpriteBatch GPU rendering active"
                : @"Metal graphics pipeline unavailable")];
    }
}

- (void)_cancelActiveTouches {
    if (![NSThread isMainThread]) return;
    auto *surface = static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    auto *touch_input = static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
    if (surface == nullptr || touch_input == nullptr) return;
    const auto current = surface->current();
    if (!current.has_value() || touch_input->surface_generation() !=
            std::optional<std::uint64_t>{current->generation}) return;

    std::uint64_t timestamp_ns = 0U;
    if (!timestamp_to_nanoseconds([[NSProcessInfo processInfo] systemUptime], timestamp_ns)) {
        [self _reportTouchError:Storm::Error::make(
            Storm::ErrorCode::Internal, "Could not read a monotonic timestamp while cancelling iOS touches.")];
        return;
    }
    const auto cancelled = cancel_touch_input(*touch_input, timestamp_ns);
    if (!cancelled) [self _reportTouchError:cancelled.error()];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if (_metalQueue != nil && _displayLink == nil) {
        _displayLink = [[CADisplayLink displayLinkWithTarget:self selector:@selector(_renderMetalFrame)] retain];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
}

- (void)_renderMetalFrame {
    if (_metalQueue == nil || _metalPipeline == nil || _spriteTexture == nil ||
        _spriteSampler == nil || _inFlightSemaphore == nil ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) {
        return;
    }
    for (std::size_t index = 0U; index < kMetalFramesInFlight; ++index) {
        if (_spriteVertexBuffers[index] == nil || _spriteIndexBuffers[index] == nil) {
            return;
        }
    }

    auto *scene = static_cast<Storm::Scene *>(_renderSceneStorage);
    auto *batch = static_cast<Storm::Rendering::SpriteBatch *>(_spriteBatchStorage);
    if (scene == nullptr || batch == nullptr) return;

    auto sprite_view = Storm::Rendering::SpriteSceneCollector2D::build_active_camera(
        *scene, *batch);
    if (!sprite_view) {
        NSLog(@"StormEngine: Scene-to-SpriteBatch collection failed: %s",
              sprite_view.error().message.c_str());
        return;
    }
    const auto &geometry = sprite_view.value();
    if (geometry.sprite_count > kDemoSpriteCapacity ||
        geometry.vertices.size() > kDemoSpriteCapacity * 4U ||
        geometry.indices.size() > kDemoSpriteCapacity * 6U) {
        NSLog(@"StormEngine: SpriteBatch exceeded the bounded Metal demo buffer capacity.");
        return;
    }
    if (geometry.sprite_count == 0U) return;

    const NSUInteger frameIndex = _frameBufferIndex;
    id<MTLBuffer> vertexBuffer = _spriteVertexBuffers[frameIndex];
    id<MTLBuffer> indexBuffer = _spriteIndexBuffers[frameIndex];
    if (vertexBuffer == nil || indexBuffer == nil) return;

    if (dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER) != 0L) {
        return;
    }
    MetalFrameSemaphoreReservation frame_slot{_inFlightSemaphore};
    _frameBufferIndex = (_frameBufferIndex + 1U) % kMetalFramesInFlight;

    const auto drawable = [(CAMetalLayer *)self.view.layer nextDrawable];
    if (drawable == nil) return;

    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.035, 0.055, 0.09, 1.0);

    id<MTLCommandBuffer> command = [_metalQueue commandBuffer];
    if (command == nil) return;
    id<MTLRenderCommandEncoder> encoder =
        [command renderCommandEncoderWithDescriptor:pass];
    if (encoder == nil) return;

    std::array<MetalSpriteVertex, kDemoSpriteCapacity * 4U> vertices{};
    for (std::size_t index = 0U; index < geometry.vertices.size(); ++index) {
        const auto &source = geometry.vertices[index];
        constexpr float inverse_channel = 1.0F / 255.0F;
        vertices[index] = MetalSpriteVertex{
            .position = {source.position.x, source.position.y},
            .uv = {source.uv.x, source.uv.y},
            .color = {
                static_cast<float>(source.color.r) * inverse_channel,
                static_cast<float>(source.color.g) * inverse_channel,
                static_cast<float>(source.color.b) * inverse_channel,
                static_cast<float>(source.color.a) * inverse_channel,
            },
        };
    }
    std::memcpy([vertexBuffer contents], vertices.data(),
                geometry.vertices.size() * sizeof(MetalSpriteVertex));
    std::memcpy([indexBuffer contents], geometry.indices.data(),
                geometry.indices.size_bytes());

    [encoder setRenderPipelineState:_metalPipeline];
    [encoder setVertexBuffer:vertexBuffer offset:0U atIndex:0U];
    [encoder setFragmentSamplerState:_spriteSampler atIndex:0U];
    [encoder setFragmentTexture:_spriteTexture atIndex:0U];
    const MTLViewport viewport{
        0.0, 0.0,
        static_cast<double>(drawable.texture.width),
        static_cast<double>(drawable.texture.height),
        0.0, 1.0
    };
    [encoder setViewport:viewport];

    bool invalid_range = false;
    for (const auto &range : geometry.ranges) {
        const std::size_t first_index = static_cast<std::size_t>(range.first_index);
        const std::size_t index_count = static_cast<std::size_t>(range.index_count);
        if (range.texture.id != 1U || range.texture.generation != 1U ||
            index_count == 0U || index_count % 3U != 0U ||
            first_index > geometry.indices.size() ||
            index_count > geometry.indices.size() - first_index) {
            invalid_range = true;
            break;
        }
        for (std::size_t index = first_index; index < first_index + index_count; ++index) {
            if (static_cast<std::size_t>(geometry.indices[index]) >=
                geometry.vertices.size()) {
                invalid_range = true;
                break;
            }
        }
        if (invalid_range) break;
        const NSUInteger offset = static_cast<NSUInteger>(
            first_index * sizeof(std::uint32_t));
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:static_cast<NSUInteger>(range.index_count)
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:indexBuffer
                     indexBufferOffset:offset];
    }
    [encoder endEncoding];

    if (invalid_range) {
        NSLog(@"StormEngine: SpriteBatch supplied an unsupported texture key or index range.");
        return;
    }

    [command presentDrawable:drawable];
    dispatch_semaphore_t semaphore = _inFlightSemaphore;
    [command addCompletedHandler:^(id<MTLCommandBuffer> completedCommand) {
        (void)completedCommand;
        dispatch_semaphore_signal(semaphore);
    }];
    frame_slot.transfer_to_command_buffer();
    [command commit];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [_displayLink invalidate];
    [_displayLink release];
    _displayLink = nil;
    [self _detachSurface];
}

- (void)_detachSurface {
    Storm::Platform::SurfaceLifecycle *surface =
        static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    if (surface == nullptr) {
        return;
    }

    const auto current = surface->current();
    if (!current.has_value()) {
        return;
    }

    auto *touch_input = static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
    if (touch_input != nullptr && touch_input->surface_generation() ==
            std::optional<std::uint64_t>{current->generation}) {
        std::uint64_t timestamp_ns = 0U;
        if (!timestamp_to_nanoseconds(
                [[NSProcessInfo processInfo] systemUptime], timestamp_ns)) {
            [self _reportTouchError:Storm::Error::make(
                Storm::ErrorCode::Internal, "Could not read a monotonic UIKit timestamp while detaching.")];
            return;
        }
        const auto input_detached = detach_touch_input(
            *touch_input, current->generation, timestamp_ns);
        if (!input_detached) {
            [self _reportTouchError:input_detached.error()];
            return;
        }
    }

    const Storm::Result<void> detached =
        surface->detach(current->native_id, current->generation);
    if (!detached) {
        [self _reportSurfaceError:detached.error()];
        return;
    }

    if (_statusLabel != nil) {
        _statusLabel.text = @"StormEngine\n\nThe iOS view surface is detached.";
    }
}

- (void)_reportTouchError:(const Storm::Error &)error {
    NSString *message = [NSString stringWithUTF8String:error.message.c_str()];
    if (message == nil) message = @"The iOS touch adapter reported an error.";
    NSLog(@"StormEngine iOS touch error (%ld): %@", static_cast<long>(error.code), message);
}

- (void)_reportSurfaceError:(const Storm::Error &)error {
    NSString *message = [NSString stringWithUTF8String:error.message.c_str()];
    if (message == nil) {
        message = @"The iOS surface adapter reported an error.";
    }
    NSLog(@"StormEngine iOS surface error (%ld): %@",
          static_cast<long>(error.code), message);
}

- (void)dealloc {
    [_displayLink invalidate];
    [_displayLink release];
    _displayLink = nil;
    [self _detachSurface];
    delete static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    _surfaceLifecycleStorage = nullptr;
    delete static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
    _touchInputStorage = nullptr;

    [_displayLink invalidate];
    [_displayLink release];
    _displayLink = nil;
    if (_inFlightSemaphore != nil) {
        for (std::size_t frame = 0U; frame < kMetalFramesInFlight; ++frame) {
            dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
        }
    }
    delete static_cast<Storm::Scene *>(_renderSceneStorage);
    _renderSceneStorage = nullptr;
    delete static_cast<Storm::Rendering::SpriteBatch *>(_spriteBatchStorage);
    _spriteBatchStorage = nullptr;

    for (std::size_t index = 0U; index < kMetalFramesInFlight; ++index) {
        [_spriteVertexBuffers[index] release];
        _spriteVertexBuffers[index] = nil;
        [_spriteIndexBuffers[index] release];
        _spriteIndexBuffers[index] = nil;
    }
    [_spriteTexture release];
    _spriteTexture = nil;
    [_spriteSampler release];
    _spriteSampler = nil;
#if !OS_OBJECT_USE_OBJC
    if (_inFlightSemaphore != nullptr) dispatch_release(_inFlightSemaphore);
#endif
    _inFlightSemaphore = nil;
    [_metalPipeline release];
    _metalPipeline = nil;
    [_metalQueue release];
    _metalQueue = nil;
    [_metalDevice release];
    _metalDevice = nil;
    [_statusLabel release];
    _statusLabel = nil;
    [super dealloc];
}

@end

@interface StormEngineApplicationSceneDelegate : StormEngineSceneDelegate

@property(nonatomic, retain) UIWindow *window;

@end

@implementation StormEngineApplicationSceneDelegate

@synthesize window = _window;

- (void)stormScene:(UIScene *)scene
    didTransitionToState:(StormEngineLifecycleState)state {
    [super stormScene:scene didTransitionToState:state];
    if (state != StormEngineLifecycleStateForegroundActive) {
        UIViewController *controller = self.window.rootViewController;
        if ([controller isKindOfClass:[StormEngineHostViewController class]]) {
            [(StormEngineHostViewController *)controller _cancelActiveTouches];
        }
    }
}

- (void)stormApplicationWillTerminate {
    [super stormApplicationWillTerminate];
    UIViewController *controller = self.window.rootViewController;
    if ([controller isKindOfClass:[StormEngineHostViewController class]]) {
        [(StormEngineHostViewController *)controller _cancelActiveTouches];
    }
}

- (void)scene:(UIScene *)scene
    willConnectToSession:(UISceneSession *)session
                 options:(UISceneConnectionOptions *)connectionOptions {
    (void)session;
    (void)connectionOptions;

    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }

    UIWindow *newWindow =
        [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    StormEngineHostViewController *rootController =
        [[StormEngineHostViewController alloc] init];
    if (rootController == nil) {
        [newWindow release];
        NSLog(@"StormEngine could not allocate its iOS surface lifecycle state.");
        return;
    }

    newWindow.rootViewController = rootController;
    self.window = newWindow;
    [newWindow makeKeyAndVisible];

    [rootController release];
    [newWindow release];
}

- (void)dealloc {
    [_window release];
    _window = nil;
    [super dealloc];
}

@end

@interface StormEngineAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation StormEngineAppDelegate

- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                                   options:(UISceneConnectionOptions *)options {
    (void)application;
    (void)options;

    UISceneConfiguration *configuration = [[UISceneConfiguration alloc]
        initWithName:@"StormEngine"
        sessionRole:connectingSceneSession.role];
    configuration.delegateClass = StormEngineApplicationSceneDelegate.class;
    return [configuration autorelease];
}

@end
