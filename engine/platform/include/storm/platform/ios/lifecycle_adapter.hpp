#pragma once

#include "storm/platform/lifecycle.hpp"

namespace Storm::Platform::IOS {

// Normalized callbacks from one UIKit scene lifecycle stream.
// Each independently managed UIScene must use its own LifecycleStateMachine.
enum class SceneEvent {
    WillEnterForeground,
    DidBecomeActive,
    WillResignActive,
    DidEnterBackground,
};

// Application termination is app-scoped rather than scene-scoped. A host
// application may forward this best-effort callback to each owned scene state.
enum class ApplicationEvent {
    WillTerminate,
};

// Applies one serialized UIScene lifecycle callback to its state machine.
// Call this on the scene adapter's owning thread.
[[nodiscard]] Result<void> apply_scene_event(
    LifecycleStateMachine& lifecycle,
    SceneEvent event);

// Applies an app-scoped UIKit lifecycle callback to one scene's state.
// Termination callbacks are best-effort and must not be the only save path.
[[nodiscard]] Result<void> apply_application_event(
    LifecycleStateMachine& lifecycle,
    ApplicationEvent event);

} // namespace Storm::Platform::IOS
