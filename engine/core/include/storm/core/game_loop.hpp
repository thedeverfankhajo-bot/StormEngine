#pragma once

#include "storm/core/fixed_step_scheduler.hpp"
#include "storm/foundation/result.hpp"

#include <chrono>

namespace Storm {

// The Core loop deliberately depends on callbacks, not on Android/iOS APIs.
// Implementations are supplied by the runtime/application layer.
class IGameLoopCallbacks {
public:
    virtual ~IGameLoopCallbacks() = default;

    // Called once for each eligible runtime frame before process_input().
    // The timestamp uses the same monotonic nanosecond domain as native input.
    // Direct GameLoop users may keep the default no-op implementation.
    [[nodiscard]] virtual Result<void> begin_frame(
        std::chrono::nanoseconds monotonic_timestamp) {
        (void)monotonic_timestamp;
        return Result<void>::success();
    }

    [[nodiscard]] virtual Result<void> process_input() = 0;
    [[nodiscard]] virtual Result<void> process_events() = 0;
    [[nodiscard]] virtual Result<void> fixed_update(
        std::chrono::nanoseconds fixed_delta) = 0;
    [[nodiscard]] virtual Result<void> variable_update(
        std::chrono::nanoseconds accepted_frame_delta) = 0;
    [[nodiscard]] virtual Result<void> render(double interpolation_alpha) = 0;
};

// Orchestrates one frame using a caller-supplied elapsed duration.
// Lifecycle gating and clock acquisition remain responsibilities of the
// platform/runtime layer. Call reset_timing() when resuming after suspension.
class GameLoop final {
public:
    [[nodiscard]] static Result<GameLoop> create(FixedStepConfig config);

    // Order: input, events, zero-or-more fixed updates, variable update, render.
    // A callback failure aborts the frame and is returned with phase context.
    // Fixed-step time has already been accounted for when simulation callbacks
    // begin; callers must not replay the same frame delta after an update error.
    [[nodiscard]] Result<FixedStepAdvance> tick(
        std::chrono::nanoseconds frame_delta,
        IGameLoopCallbacks& callbacks);

    // Clears accumulated time so background/suspended time is not simulated.
    void reset_timing() noexcept;

    [[nodiscard]] const FixedStepConfig& config() const noexcept;

private:
    explicit GameLoop(FixedStepScheduler scheduler) noexcept;

    FixedStepScheduler scheduler_;
};

} // namespace Storm
