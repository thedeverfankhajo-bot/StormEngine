#include "storm/platform/android/lifecycle_adapter.hpp"

namespace Storm::Platform::Android {
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
        "An Android activity event cannot revive a terminating lifecycle"));
}

} // namespace

Result<void> apply_activity_event(
    LifecycleStateMachine& lifecycle,
    ActivityEvent event) {
    if (lifecycle.is_terminating()) {
        if (event == ActivityEvent::Destroyed) {
            return Result<void>::success();
        }
        return invalid_terminal_event();
    }

    switch (event) {
        case ActivityEvent::Started:
        case ActivityEvent::Resumed: {
            const LifecycleState current = lifecycle.state();
            if (current == LifecycleState::Created ||
                current == LifecycleState::Background) {
                return transition_to(lifecycle, LifecycleState::ForegroundInactive);
            }
            return Result<void>::success();
        }

        case ActivityEvent::Paused:
        case ActivityEvent::FocusLost:
            if (lifecycle.state() == LifecycleState::ForegroundActive) {
                return transition_to(lifecycle, LifecycleState::ForegroundInactive);
            }
            return Result<void>::success();

        case ActivityEvent::Stopped: {
            if (lifecycle.state() == LifecycleState::ForegroundActive) {
                const Result<void> inactive =
                    transition_to(lifecycle, LifecycleState::ForegroundInactive);
                if (!inactive) {
                    return inactive;
                }
            }

            if (lifecycle.state() == LifecycleState::Background) {
                return Result<void>::success();
            }
            return transition_to(lifecycle, LifecycleState::Background);
        }

        case ActivityEvent::FocusGained: {
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

        case ActivityEvent::Destroyed:
            return transition_to(lifecycle, LifecycleState::Terminating);
    }

    return Result<void>::failure(Storm::Error::make(
        Storm::ErrorCode::InvalidArgument,
        "The Android activity event value is not recognized"));
}

} // namespace Storm::Platform::Android
