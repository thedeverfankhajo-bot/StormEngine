#pragma once

#include "storm/foundation/result.hpp"

#include <cstdint>

namespace Storm::Platform {

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

class LifecycleStateMachine final {
public:
    LifecycleStateMachine() = default;

    [[nodiscard]] Result<LifecycleTransition> transition_to(LifecycleState next);

    [[nodiscard]] LifecycleState state() const noexcept;
    // Monotonically increments for each successful, non-idempotent transition.
    [[nodiscard]] std::uint64_t revision() const noexcept;
    [[nodiscard]] bool is_foreground() const noexcept;
    [[nodiscard]] bool can_run_game_loop() const noexcept;
    [[nodiscard]] bool is_terminating() const noexcept;

private:
    [[nodiscard]] static bool is_transition_allowed(
        LifecycleState current,
        LifecycleState next) noexcept;

    LifecycleState state_{LifecycleState::Created};
    std::uint64_t revision_{0U};
};

} // namespace Storm::Platform
