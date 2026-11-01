#pragma once

#include "storm/foundation/result.hpp"

namespace Storm::Platform {

// A normalized state for one native presentation lifecycle stream.
// An iOS adapter may own one instance per UIScene; an Android adapter
// typically owns one instance per Activity.
enum class LifecycleState {
    Created,
    ForegroundInactive,
    ForegroundActive,
    Background,
    Terminating,
};

struct LifecycleTransition final {
    LifecycleState previous{LifecycleState::Created};
    LifecycleState current{LifecycleState::Created};
    bool changed{false};
};

// A small, single-thread-affine state model. Platform adapters must serialize
// calls on their native lifecycle callback queue before forwarding transitions.
class LifecycleStateMachine final {
public:
    LifecycleStateMachine() = default;

    [[nodiscard]] Result<LifecycleTransition> transition_to(LifecycleState next);

    [[nodiscard]] LifecycleState state() const noexcept;
    [[nodiscard]] bool is_foreground() const noexcept;
    [[nodiscard]] bool can_run_game_loop() const noexcept;
    [[nodiscard]] bool is_terminating() const noexcept;

private:
    [[nodiscard]] static bool is_transition_allowed(
        LifecycleState current,
        LifecycleState next) noexcept;

    LifecycleState state_{LifecycleState::Created};
};

} // namespace Storm::Platform
