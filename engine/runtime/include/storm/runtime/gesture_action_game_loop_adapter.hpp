#pragma once

#include "storm/core/game_loop.hpp"
#include "storm/input/touch_input.hpp"
#include "storm/runtime/gesture_action_input_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>

namespace Storm::Runtime {

// Called synchronously on the game-loop thread when an input frame produces
// one or more mapped actions. The span is non-owning and expires when this
// callback returns.
class IGameActionFrameConsumer {
public:
    virtual ~IGameActionFrameConsumer() = default;

    [[nodiscard]] virtual Result<void> process_game_actions(
        std::span<const Input::InputActionEvent> actions) = 0;
};

// Connects native TouchInput frames, gesture recognition, and action mapping
// to an application-owned gameplay consumer during Core's input phase.
//
// MobileFrameDriver supplies the absolute monotonic frame timestamp through
// begin_frame(). This is required for time-based gestures such as LongPress.
// The adapter is single-thread-affine and holds non-owning references; all
// referenced objects must outlive it and be used on the same serialized thread.
class GestureActionGameLoopAdapter final : public IGameLoopCallbacks {
public:
    GestureActionGameLoopAdapter(
        IGameLoopCallbacks& delegate,
        Input::TouchInput& touch_input,
        GestureActionInputPipeline& input_pipeline,
        IGameActionFrameConsumer& action_consumer) noexcept;

    GestureActionGameLoopAdapter(const GestureActionGameLoopAdapter&) = delete;
    GestureActionGameLoopAdapter& operator=(const GestureActionGameLoopAdapter&) = delete;
    GestureActionGameLoopAdapter(GestureActionGameLoopAdapter&&) = delete;
    GestureActionGameLoopAdapter& operator=(GestureActionGameLoopAdapter&&) = delete;

    [[nodiscard]] Result<void> begin_frame(
        std::chrono::nanoseconds monotonic_timestamp) override;
    [[nodiscard]] Result<void> process_input() override;
    [[nodiscard]] Result<void> process_events() override;
    [[nodiscard]] Result<void> fixed_update(
        std::chrono::nanoseconds fixed_delta) override;
    [[nodiscard]] Result<void> variable_update(
        std::chrono::nanoseconds accepted_frame_delta) override;
    [[nodiscard]] Result<void> render(double interpolation_alpha) override;

private:
    IGameLoopCallbacks& delegate_;
    Input::TouchInput& touch_input_;
    GestureActionInputPipeline& input_pipeline_;
    IGameActionFrameConsumer& action_consumer_;
    std::optional<std::uint64_t> frame_timestamp_ns_;
};

} // namespace Storm::Runtime
