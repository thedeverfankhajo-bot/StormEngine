#include "storm/platform/ios/lifecycle_adapter.hpp"

namespace Storm::Platform::IOS {
namespace {

[[nodiscard]] Result<void> transition_to(
    LifecycleStateMachine& lifecycle,
    LifecycleState next) {
    const Result<LifecycleTransition> transition = lifecycle.transition_to(next);
    if (!transition) {
        return Result<void>::failure(transition.error());
    }
    return Result<void>::success();
}

[[nodiscard]] Result<void> invalid_terminal_event() {
    return Result<void>::failure(Storm::Error::make(
        Storm::ErrorCode::InvalidState,
        "A UIKit scene event cannot revive a terminating lifecycle"));
}

[[nodiscard]] Result<void> invalid_scene_event() {
    return Result<void>::failure(Storm::Error::make(
        Storm::ErrorCode::InvalidArgument,
        "The UIKit scene event value is not recognized"));
}

} // namespace

Result<void> apply_scene_event(
    LifecycleStateMachine& lifecycle,
    SceneEvent event) {
    if (lifecycle.is_terminating()) {
        return invalid_terminal_event();
    }

    switch (event) {
        case SceneEvent::WillEnterForeground: {
            const LifecycleState current = lifecycle.state();
            if (current == LifecycleState::Created ||
                current == LifecycleState::Background) {
                return transition_to(lifecycle, LifecycleState::ForegroundInactive);
            }
            return Result<void>::success();
        }

        case SceneEvent::DidBecomeActive: {
            const LifecycleState current = lifecycle.state();
            if (current == LifecycleState::Created ||
                current == LifecycleState::Background) {
                const Result<void> inactive =
                    transition_to(lifecycle, LifecycleState::ForegroundInactive);
                if (!inactive) {
                    return inactive;
                }
            }

            if (lifecycle.state() == LifecycleState::ForegroundActive) {
                return Result<void>::success();
            }
            return transition_to(lifecycle, LifecycleState::ForegroundActive);
        }

        case SceneEvent::WillResignActive:
            if (lifecycle.state() == LifecycleState::ForegroundActive) {
                return transition_to(lifecycle, LifecycleState::ForegroundInactive);
            }
            return Result<void>::success();

        case SceneEvent::DidEnterBackground: {
            const LifecycleState current = lifecycle.state();
            if (current == LifecycleState::ForegroundActive) {
                const Result<void> inactive =
                    transition_to(lifecycle, LifecycleState::ForegroundInactive);
                if (!inactive) {
                    return inactive;
                }
            }

            const LifecycleState normalized = lifecycle.state();
            if (normalized == LifecycleState::Created ||
                normalized == LifecycleState::ForegroundInactive) {
                return transition_to(lifecycle, LifecycleState::Background);
            }
            return Result<void>::success();
        }
    }

    return invalid_scene_event();
}

Result<void> apply_application_event(
    LifecycleStateMachine& lifecycle,
    ApplicationEvent event) {
    switch (event) {
        case ApplicationEvent::WillTerminate:
            if (lifecycle.is_terminating()) {
                return Result<void>::success();
            }
            return transition_to(lifecycle, LifecycleState::Terminating);
    }

    return Result<void>::failure(Storm::Error::make(
        Storm::ErrorCode::InvalidArgument,
        "The UIKit application event value is not recognized"));
}

} // namespace Storm::Platform::IOS
