#import "storm/ios/scene_lifecycle_delegate.h"

#include "storm/platform/ios/lifecycle_adapter.hpp"

#include <new>

using Storm::Platform::LifecycleState;
using Storm::Platform::LifecycleStateMachine;
using Storm::Platform::IOS::ApplicationEvent;
using Storm::Platform::IOS::SceneEvent;
using Storm::Platform::IOS::apply_application_event;
using Storm::Platform::IOS::apply_scene_event;

namespace {

[[nodiscard]] StormEngineLifecycleState to_public_state(LifecycleState state) noexcept {
    switch (state) {
        case LifecycleState::Created:
            return StormEngineLifecycleStateCreated;
        case LifecycleState::ForegroundInactive:
            return StormEngineLifecycleStateForegroundInactive;
        case LifecycleState::ForegroundActive:
            return StormEngineLifecycleStateForegroundActive;
        case LifecycleState::Background:
            return StormEngineLifecycleStateBackground;
        case LifecycleState::Terminating:
            return StormEngineLifecycleStateTerminating;
    }
    return StormEngineLifecycleStateTerminating;
}

} // namespace

@interface StormEngineSceneDelegate ()
- (LifecycleStateMachine *)_stormLifecycleMachine;
- (void)_reportLifecycleError:(const Storm::Error &)error;
- (void)_applySceneEvent:(SceneEvent)event scene:(UIScene *)scene;
- (void)_applicationWillTerminate:(NSNotification *)notification;
@end

@implementation StormEngineSceneDelegate

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _stormEngineLifecycleStorage = new (std::nothrow) LifecycleStateMachine();
    if (_stormEngineLifecycleStorage == nullptr) {
        return nil;
    }

    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(_applicationWillTerminate:)
               name:UIApplicationWillTerminateNotification
             object:nil];
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    delete static_cast<LifecycleStateMachine *>(_stormEngineLifecycleStorage);
    _stormEngineLifecycleStorage = nullptr;
}

- (LifecycleStateMachine *)_stormLifecycleMachine {
    return static_cast<LifecycleStateMachine *>(_stormEngineLifecycleStorage);
}

- (StormEngineLifecycleState)stormLifecycleState {
    const LifecycleStateMachine *machine =
        static_cast<const LifecycleStateMachine *>(_stormEngineLifecycleStorage);
    return machine == nullptr
        ? StormEngineLifecycleStateCreated
        : to_public_state(machine->state());
}

- (void)_reportLifecycleError:(const Storm::Error &)error {
    NSString *message = [NSString stringWithUTF8String:error.message.c_str()];
    if (message == nil) {
        message = @"The lifecycle adapter reported an error.";
    }
    [self stormLifecycleDidFailWithCode:static_cast<NSInteger>(error.code)
                                message:message];
}

- (void)_applySceneEvent:(SceneEvent)event scene:(UIScene *)scene {
    if (![NSThread isMainThread]) {
        [self stormLifecycleDidFailWithCode:static_cast<NSInteger>(Storm::ErrorCode::InvalidState)
                                    message:@"UIKit lifecycle callbacks must be processed on the main thread."];
        return;
    }

    LifecycleStateMachine *machine = [self _stormLifecycleMachine];
    if (machine == nullptr) {
        [self stormLifecycleDidFailWithCode:static_cast<NSInteger>(Storm::ErrorCode::ResourceExhausted)
                                    message:@"The scene lifecycle state could not be allocated."];
        return;
    }

    const LifecycleState previous = machine->state();
    const Storm::Result<void> result = apply_scene_event(*machine, event);
    if (!result) {
        [self _reportLifecycleError:result.error()];
        return;
    }

    const LifecycleState current = machine->state();
    if (current != previous) {
        [self stormScene:scene didTransitionToState:to_public_state(current)];
    }
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
    [self _applySceneEvent:SceneEvent::WillEnterForeground scene:scene];
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
    [self _applySceneEvent:SceneEvent::DidBecomeActive scene:scene];
}

- (void)sceneWillResignActive:(UIScene *)scene {
    [self _applySceneEvent:SceneEvent::WillResignActive scene:scene];
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
    [self _applySceneEvent:SceneEvent::DidEnterBackground scene:scene];
}

- (void)_applicationWillTerminate:(NSNotification *)notification {
    (void)notification;
    if (![NSThread isMainThread]) {
        [self stormLifecycleDidFailWithCode:static_cast<NSInteger>(Storm::ErrorCode::InvalidState)
                                    message:@"The application termination notification must be processed on the main thread."];
        return;
    }

    LifecycleStateMachine *machine = [self _stormLifecycleMachine];
    if (machine == nullptr) {
        [self stormLifecycleDidFailWithCode:static_cast<NSInteger>(Storm::ErrorCode::ResourceExhausted)
                                    message:@"The scene lifecycle state is unavailable during application termination."];
        return;
    }

    const LifecycleState previous = machine->state();
    const Storm::Result<void> result =
        apply_application_event(*machine, ApplicationEvent::WillTerminate);
    if (!result) {
        [self _reportLifecycleError:result.error()];
        return;
    }

    if (machine->state() != previous) {
        [self stormApplicationWillTerminate];
    }
}

- (void)stormScene:(UIScene *)scene
    didTransitionToState:(StormEngineLifecycleState)state {
    (void)scene;
    (void)state;
    // Intentionally empty extension point for the host application's runtime.
}

- (void)stormApplicationWillTerminate {
    // Intentionally empty best-effort extension point for the host application.
}

- (void)stormLifecycleDidFailWithCode:(NSInteger)code
                              message:(NSString *)message {
    NSLog(@"StormEngine iOS lifecycle error (%ld): %@", (long)code, message);
}

@end
