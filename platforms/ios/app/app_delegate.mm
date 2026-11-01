#import <UIKit/UIKit.h>

#import "storm/ios/scene_lifecycle_delegate.h"

#include "storm/platform/surface_lifecycle.hpp"

#include <cmath>
#include <cstdint>
#include <new>

namespace {

constexpr Storm::Platform::NativeSurfaceId kIOSHostSurfaceId = 1U;

} // namespace

@interface StormEngineHostViewController : UIViewController {
@private
    void *_surfaceLifecycleStorage;
    UILabel *_statusLabel;
}

- (void)_detachSurface;
- (void)_reportSurfaceError:(const Storm::Error &)error;

@end

@implementation StormEngineHostViewController

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

    _surfaceLifecycleStorage = surface;
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor systemBackgroundColor];

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
    _statusLabel.text = [NSString stringWithFormat:
        @"StormEngine\n\nNative iOS view surface attached\n"
        "%u × %u pixels · %.2f pixels per point\n\n"
        "Rendering, touch input, and the game runtime are not integrated yet.",
        static_cast<unsigned int>(descriptor.extent.width),
        static_cast<unsigned int>(descriptor.extent.height),
        descriptor.pixels_per_logical_unit];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
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
