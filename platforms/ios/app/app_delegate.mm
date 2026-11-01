#import <UIKit/UIKit.h>

#import "storm/ios/scene_lifecycle_delegate.h"

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
    UIViewController *rootController = [[UIViewController alloc] init];
    rootController.view.backgroundColor = [UIColor systemBackgroundColor];

    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    statusLabel.numberOfLines = 0;
    statusLabel.textAlignment = NSTextAlignmentCenter;
    statusLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    statusLabel.text = @"StormEngine\n\nThe iOS native host is running. Rendering, touch input, and the game runtime are not integrated yet.";
    statusLabel.accessibilityIdentifier = @"stormengine.host.status";
    statusLabel.translatesAutoresizingMaskIntoConstraints = NO;

    [rootController.view addSubview:statusLabel];
    [NSLayoutConstraint activateConstraints:@[
        [statusLabel.leadingAnchor
            constraintGreaterThanOrEqualToAnchor:rootController.view.safeAreaLayoutGuide.leadingAnchor
                                         constant:24.0],
        [statusLabel.trailingAnchor
            constraintLessThanOrEqualToAnchor:rootController.view.safeAreaLayoutGuide.trailingAnchor
                                      constant:-24.0],
        [statusLabel.centerXAnchor
            constraintEqualToAnchor:rootController.view.safeAreaLayoutGuide.centerXAnchor],
        [statusLabel.centerYAnchor
            constraintEqualToAnchor:rootController.view.safeAreaLayoutGuide.centerYAnchor]
    ]];

    newWindow.rootViewController = rootController;
    self.window = newWindow;
    [newWindow makeKeyAndVisible];

    [statusLabel release];
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
