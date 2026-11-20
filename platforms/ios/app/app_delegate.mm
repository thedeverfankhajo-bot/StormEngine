#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#import "storm/ios/scene_lifecycle_delegate.h"

#include "storm/platform/surface_lifecycle.hpp"
#include "storm/input/touch_input.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

namespace {

constexpr Storm::Platform::NativeSurfaceId kIOSHostSurfaceId = 1U;

struct alignas(16) MetalSmokeVertex final {
    float position[4];
    float color[4];
};
static_assert(sizeof(MetalSmokeVertex) == 32U);

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

    _surfaceLifecycleStorage = surface;
    _touchInputStorage = touch_input;
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
                 "struct VertexData { float4 position; float4 color; };\n"
                 "struct RasterData { float4 position [[position]]; float4 color; };\n"
                 "vertex RasterData storm_vertex(\n"
                 "    uint vertex_id [[vertex_id]],\n"
                 "    const device VertexData* vertices [[buffer(0)]]) {\n"
                 "    RasterData output;\n"
                 "    output.position = vertices[vertex_id].position;\n"
                 "    output.color = vertices[vertex_id].color;\n"
                 "    return output;\n"
                 "}\n"
                 "fragment float4 storm_fragment(RasterData input [[stage_in]]) {\n"
                 "    return input.color;\n"
                 "}\n";

            NSError *libraryError = nil;
            id<MTLLibrary> library = [_metalDevice
                newLibraryWithSource:shaderSource options:nil error:&libraryError];
            if (library == nil) {
                NSLog(@"StormEngine: Metal shader compilation failed: %@",
                      libraryError);
            } else {
                id<MTLFunction> vertexFunction =
                    [library newFunctionWithName:@"storm_vertex"];
                id<MTLFunction> fragmentFunction =
                    [library newFunctionWithName:@"storm_fragment"];

                if (vertexFunction == nil || fragmentFunction == nil) {
                    NSLog(@"StormEngine: Metal shader library is missing an entry point.");
                } else {
                    MTLRenderPipelineDescriptor *descriptor =
                        [[MTLRenderPipelineDescriptor alloc] init];
                    descriptor.vertexFunction = vertexFunction;
                    descriptor.fragmentFunction = fragmentFunction;
                    descriptor.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;

                    NSError *pipelineError = nil;
                    _metalPipeline = [_metalDevice
                        newRenderPipelineStateWithDescriptor:descriptor
                                                       error:&pipelineError];
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
        [_statusLabel.centerYAnchor
            constraintEqualToAnchor:self.view.safeAreaLayoutGuide.centerYAnchor]
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
            : (_metalPipeline != nil
                ? @"Metal pipeline ready · GPU triangle draw active"
                : @"Metal clear-only fallback · pipeline unavailable"))];
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
                ? @"Metal GPU triangle draw active"
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
    if (_metalQueue == nil ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) {
        return;
    }

    CAMetalLayer *layer = (CAMetalLayer *)self.view.layer;
    id<CAMetalDrawable> drawable = [layer nextDrawable];
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

    if (_metalPipeline != nil) {
        constexpr std::array<MetalSmokeVertex, 3U> vertices{{
            {{0.0F, 0.62F, 0.0F, 1.0F}, {1.0F, 0.32F, 0.18F, 1.0F}},
            {{-0.58F, -0.48F, 0.0F, 1.0F}, {0.18F, 0.78F, 0.98F, 1.0F}},
            {{0.58F, -0.48F, 0.0F, 1.0F}, {0.58F, 0.42F, 0.98F, 1.0F}}
        }};

        [encoder setRenderPipelineState:_metalPipeline];
        [encoder setVertexBytes:vertices.data()
                         length:sizeof(vertices)
                        atIndex:0U];
        const MTLViewport viewport{
            0.0, 0.0,
            static_cast<double>(drawable.texture.width),
            static_cast<double>(drawable.texture.height),
            0.0, 1.0
        };
        [encoder setViewport:viewport];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                   vertexStart:0U
                   vertexCount:static_cast<NSUInteger>(vertices.size())];
    }

    [encoder endEncoding];
    [command presentDrawable:drawable];
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
    [self _detachSurface];
    delete static_cast<Storm::Platform::SurfaceLifecycle *>(_surfaceLifecycleStorage);
    _surfaceLifecycleStorage = nullptr;
    delete static_cast<Storm::Input::TouchInput *>(_touchInputStorage);
    _touchInputStorage = nullptr;
    [_displayLink invalidate];
    [_displayLink release];
    _displayLink = nil;
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
