#include "storm/core/game_loop.hpp"

#include <string>
#include <utility>

namespace Storm {
namespace {

[[nodiscard]] Error add_phase_context(const Error& error, const char* phase) {
    return Error::make(
        error.code,
        std::string("GameLoop::") + phase + " failed: " + error.message);
}

[[nodiscard]] Result<FixedStepAdvance> callback_failure(
    const Error& error,
    const char* phase) {
    return Result<FixedStepAdvance>::failure(add_phase_context(error, phase));
}

} // namespace

GameLoop::GameLoop(FixedStepScheduler scheduler) noexcept
    : scheduler_(std::move(scheduler)) {}

Result<GameLoop> GameLoop::create(FixedStepConfig config) {
    Result<FixedStepScheduler> scheduler =
        FixedStepScheduler::create(std::move(config));
    if (!scheduler) {
        return Result<GameLoop>::failure(scheduler.error());
    }

    return Result<GameLoop>::success(
        GameLoop{std::move(scheduler).value()});
}

Result<FixedStepAdvance> GameLoop::tick(
    std::chrono::nanoseconds frame_delta,
    IGameLoopCallbacks& callbacks) {
    if (frame_delta.count() < 0) {
        return Result<FixedStepAdvance>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "GameLoop::tick requires a non-negative frame_delta"));
    }

    const Result<void> input_result = callbacks.process_input();
    if (!input_result) {
        return callback_failure(input_result.error(), "process_input");
    }

    const Result<void> event_result = callbacks.process_events();
    if (!event_result) {
        return callback_failure(event_result.error(), "process_events");
    }

    const Result<FixedStepAdvance> timing_result = scheduler_.advance(frame_delta);
    if (!timing_result) {
        return Result<FixedStepAdvance>::failure(timing_result.error());
    }

    const FixedStepAdvance timing = timing_result.value();
    const std::chrono::nanoseconds fixed_delta = scheduler_.config().fixed_step;

    for (std::uint32_t step = 0U; step < timing.step_count; ++step) {
        const Result<void> update_result = callbacks.fixed_update(fixed_delta);
        if (!update_result) {
            return callback_failure(update_result.error(), "fixed_update");
        }
    }

    const Result<void> variable_result =
        callbacks.variable_update(timing.accepted_frame_delta);
    if (!variable_result) {
        return callback_failure(variable_result.error(), "variable_update");
    }

    const Result<void> render_result =
        callbacks.render(timing.interpolation_alpha);
    if (!render_result) {
        return callback_failure(render_result.error(), "render");
    }

    return Result<FixedStepAdvance>::success(timing);
}

void GameLoop::reset_timing() noexcept {
    scheduler_.reset();
}

const FixedStepConfig& GameLoop::config() const noexcept {
    return scheduler_.config();
}

} // namespace Storm
