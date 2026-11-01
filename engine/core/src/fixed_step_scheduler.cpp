#include "storm/core/fixed_step_scheduler.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace Storm {

FixedStepScheduler::FixedStepScheduler(FixedStepConfig config) noexcept
    : config_(std::move(config)) {}

Result<FixedStepScheduler> FixedStepScheduler::create(FixedStepConfig config) {
    if (config.fixed_step.count() <= 0) {
        return Result<FixedStepScheduler>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "fixed_step must be greater than zero"));
    }

    if (config.maximum_frame_delta.count() <= 0) {
        return Result<FixedStepScheduler>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "maximum_frame_delta must be greater than zero"));
    }

    if (config.maximum_steps_per_frame == 0U) {
        return Result<FixedStepScheduler>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "maximum_steps_per_frame must be greater than zero"));
    }

    return Result<FixedStepScheduler>::success(
        FixedStepScheduler{std::move(config)});
}

Result<FixedStepAdvance> FixedStepScheduler::advance(
    std::chrono::nanoseconds frame_delta) {
    using Duration = std::chrono::nanoseconds;
    using Rep = Duration::rep;

    if (frame_delta.count() < 0) {
        return Result<FixedStepAdvance>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "frame_delta cannot be negative"));
    }

    const Duration accepted_delta =
        std::min(frame_delta, config_.maximum_frame_delta);
    const Duration clamped_delta = frame_delta - accepted_delta;

    const Rep current_count = accumulator_.count();
    const Rep accepted_count = accepted_delta.count();
    const Rep maximum_count = std::numeric_limits<Rep>::max();

    if (accepted_count > maximum_count - current_count) {
        return Result<FixedStepAdvance>::failure(Storm::Error::make(
            Storm::ErrorCode::ResourceExhausted,
            "Fixed-step accumulator would overflow its duration representation"));
    }

    Duration pending{current_count + accepted_count};
    FixedStepAdvance result{
        .step_count = 0U,
        .interpolation_alpha = 0.0,
        .accepted_frame_delta = accepted_delta,
        .clamped_frame_delta = clamped_delta,
        .discarded_backlog = Duration{0},
    };

    while (pending >= config_.fixed_step &&
           result.step_count < config_.maximum_steps_per_frame) {
        pending -= config_.fixed_step;
        ++result.step_count;
    }

    // If the catch-up budget was exhausted, discard whole pending steps while
    // preserving the sub-step remainder used for render interpolation.
    if (pending >= config_.fixed_step) {
        const Rep remainder_count = pending.count() % config_.fixed_step.count();
        const Duration remainder{remainder_count};
        result.discarded_backlog = pending - remainder;
        pending = remainder;
    }

    accumulator_ = pending;
    result.interpolation_alpha =
        static_cast<double>(accumulator_.count()) /
        static_cast<double>(config_.fixed_step.count());

    return Result<FixedStepAdvance>::success(result);
}

void FixedStepScheduler::reset() noexcept {
    accumulator_ = std::chrono::nanoseconds{0};
}

const FixedStepConfig& FixedStepScheduler::config() const noexcept {
    return config_;
}

std::chrono::nanoseconds FixedStepScheduler::remaining_accumulator() const noexcept {
    return accumulator_;
}

} // namespace Storm
