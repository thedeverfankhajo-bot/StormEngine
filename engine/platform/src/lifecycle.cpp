#include "storm/platform/lifecycle.hpp"

namespace Storm::Platform {

Result<LifecycleTransition> LifecycleStateMachine::transition_to(LifecycleState next) {
    const LifecycleState previous = state_;
    if (next == previous) {
        return Result<LifecycleTransition>::success(LifecycleTransition{
            .previous = previous,
            .current = next,
            .changed = false,
        });
    }

    if (!is_transition_allowed(previous, next)) {
        return Result<LifecycleTransition>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidState,
            "The requested platform lifecycle transition is not allowed"));
    }

    state_ = next;
    return Result<LifecycleTransition>::success(LifecycleTransition{
        .previous = previous,
        .current = next,
        .changed = true,
    });
}

LifecycleState LifecycleStateMachine::state() const noexcept {
    return state_;
}

bool LifecycleStateMachine::is_foreground() const noexcept {
    return state_ == LifecycleState::ForegroundInactive ||
        state_ == LifecycleState::ForegroundActive;
}

bool LifecycleStateMachine::can_run_game_loop() const noexcept {
    return state_ == LifecycleState::ForegroundActive;
}

bool LifecycleStateMachine::is_terminating() const noexcept {
    return state_ == LifecycleState::Terminating;
}

bool LifecycleStateMachine::is_transition_allowed(
    LifecycleState current,
    LifecycleState next) noexcept {
    if (next == LifecycleState::Terminating) {
        return current != LifecycleState::Terminating;
    }

    switch (current) {
        case LifecycleState::Created:
            return next == LifecycleState::ForegroundInactive ||
                next == LifecycleState::Background;
        case LifecycleState::ForegroundInactive:
            return next == LifecycleState::ForegroundActive ||
                next == LifecycleState::Background;
        case LifecycleState::ForegroundActive:
            return next == LifecycleState::ForegroundInactive;
        case LifecycleState::Background:
            return next == LifecycleState::ForegroundInactive;
        case LifecycleState::Terminating:
            return false;
    }

    return false;
}

} // namespace Storm::Platform
