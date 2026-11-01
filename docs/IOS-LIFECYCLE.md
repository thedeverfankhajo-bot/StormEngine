# iOS Lifecycle Event Adapter

## Status and scope

The platform-neutral iOS lifecycle event adapter is implemented in Storm::Platform::IOS. It normalizes UIKit scene lifecycle events into the shared LifecycleStateMachine contract.

This is not a UIKit integration. The repository still has no UIApplicationDelegate, UISceneDelegate, Objective-C/Objective-C++ bridge, iOS application target, or physical-device lifecycle test. A host application must currently forward UIKit callbacks to this adapter itself.

## Scene ownership

Create one LifecycleStateMachine instance for each independently managed UIScene. Do not share one state machine across multiple scenes: one scene may be active while another is in the background.

The adapter is single-thread-affine, like the lifecycle state machine. Call it serially on the owning scene's lifecycle callback queue. It is not internally synchronized and does not enqueue callbacks.

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

The dedicated host test executable stormengine_ios_lifecycle_tests contains seven cases covering the foreground/background sequence, recovery from background, launch into background, idempotent callbacks, independent scene state, terminal app shutdown, and invalid events.

These tests validate the C++ event-normalization policy only. They do not invoke UIKit, create a UIScene, validate callback delivery order from iOS, or establish physical-device compatibility.
