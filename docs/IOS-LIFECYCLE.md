# iOS Lifecycle Event Adapter

## Status and scope

The platform-neutral iOS lifecycle event adapter is implemented in Storm::Platform::IOS. It normalizes UIKit scene lifecycle events into the shared LifecycleStateMachine contract.

The repository now includes StormEngineSceneDelegate, an Objective-C++ UIWindowSceneDelegate adapter available on iOS 13+ and compiled only for iOS. It owns one LifecycleStateMachine per scene and forwards normalized state transitions through an overridable method. This is a native scene-lifecycle integration component, not a complete iOS application: the repository still has no UIApplicationDelegate, app target, runtime event consumer, signed app, or physical-device lifecycle test.

## Scene ownership

Create one LifecycleStateMachine instance for each independently managed UIScene. Do not share one state machine across multiple scenes: one scene may be active while another is in the background.

The platform-neutral adapter is single-thread-affine. The Objective-C++ delegate receives UIKit scene callbacks on the main thread and invokes it there. The delegate checks this requirement and reports an error instead of updating state from another thread. It does not enqueue callbacks; the host runtime must consume the overridable transition hook without blocking.

## Callback mapping

| UIKit callback | Adapter event | Normalized state |
|---|---|---|
| sceneWillEnterForeground(_:) | SceneEvent::WillEnterForeground | ForegroundInactive |
| sceneDidBecomeActive(_:) | SceneEvent::DidBecomeActive | ForegroundActive |
| sceneWillResignActive(_:) | SceneEvent::WillResignActive | ForegroundInactive |
| sceneDidEnterBackground(_:) | SceneEvent::DidEnterBackground | Background |

DidBecomeActive can recover from Created or Background by normalizing through ForegroundInactive. DidEnterBackground can normalize from Created or an active state. Repeated callbacks are idempotent.

The app-scoped applicationWillTerminate(_:) notification can be forwarded to each owned scene lifecycle with apply_application_event(..., ApplicationEvent::WillTerminate). It is a best-effort notification, not a reliable persistence boundary. Save important project/game state incrementally and do not depend on this callback as the only save opportunity.

## Error behavior

- A scene callback after the lifecycle has entered Terminating returns ErrorCode::InvalidState.
- Repeated app termination events succeed.
- Unknown enum values return ErrorCode::InvalidArgument.
- Invalid transitions leave the state machine unchanged because all state updates pass through LifecycleStateMachine::transition_to().

## Validation

The dedicated host test executable stormengine_ios_lifecycle_tests contains seven cases covering the foreground/background sequence, recovery from background, launch into background, idempotent callbacks, independent scene state, terminal app shutdown, and invalid events. All seven cases passed in GCC, Clang, Apple Clang, and Clang ASan/UBSan in CI run 41304250865. The Platform library, including this adapter, also cross-compiled for Android arm64-v8a and iOS device arm64 in that workflow.

The seven host tests validate the C++ event-normalization policy only. The iOS CI build additionally compiles the Objective-C++ delegate against the iPhoneOS UIKit SDK. Cross-compilation does not invoke UIKit callbacks, create a UIScene, validate callback delivery order on an operating system, or establish physical-device compatibility.


## Host application integration

Link the host app against Storm Platform. Because the delegate class is stored in a static archive and selected by class name, the iOS consumer link must retain Objective-C classes; the CMake target propagates the -ObjC linker option.

Configure StormEngineSceneDelegate (or a subclass) as the delegate class in the host application's UISceneConfiguration. An app may subclass the delegate and override the following hook:

```objective-c
- (void)stormScene:(UIScene *)scene
    didTransitionToState:(StormEngineLifecycleState)state {
    // Forward the scene identifier and state to the app-owned runtime.
}
```

The override is called synchronously on the main thread only when the normalized state changes. It must not block. Each scene delegate owns independent state. The app-termination hook is best-effort; persistent state must be saved incrementally rather than relying on termination.
