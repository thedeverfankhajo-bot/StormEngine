#include "storm/runtime/gesture_action_game_loop_adapter.hpp"

#include <string>
#include <utility>

namespace Storm::Runtime {
namespace {

class TouchFrameReset final {
public:
    explicit TouchFrameReset(Input::TouchInput& input) noexcept
        : input_(input) {}

    TouchFrameReset(const TouchFrameReset&) = delete;
    TouchFrameReset& operator=(const TouchFrameReset&) = delete;

    ~TouchFrameReset() {
        input_.begin_frame();
    }

private:
    Input::TouchInput& input_;
};

[[nodiscard]] Error with_context(const Error& error, const char* operation) {
    return Error::make(
        error.code,
        std::string("GestureActionGameLoopAdapter::") + operation +
            " failed: " + error.message);
}

} // namespace

GestureActionGameLoopAdapter::GestureActionGameLoopAdapter(
    IGameLoopCallbacks& delegate,
    Input::TouchInput& touch_input,
    GestureActionInputPipeline& input_pipeline,
    IGameActionFrameConsumer& action_consumer) noexcept
    : delegate_(delegate),
      touch_input_(touch_input),
      input_pipeline_(input_pipeline),
      action_consumer_(action_consumer) {}

Result<void> GestureActionGameLoopAdapter::begin_frame(
    std::chrono::nanoseconds monotonic_timestamp) {
    if (monotonic_timestamp.count() < 0) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "monotonic frame timestamp cannot be negative"));
    }

    const Result<void> delegated =
        delegate_.begin_frame(monotonic_timestamp);
    if (!delegated) {
        return Result<void>::failure(
            with_context(delegated.error(), "begin_frame"));
    }

    frame_timestamp_ns_ =
        static_cast<std::uint64_t>(monotonic_timestamp.count());
    return Result<void>::success();
}

Result<void> GestureActionGameLoopAdapter::process_input() {
    const std::optional<std::uint64_t> timestamp =
        std::exchange(frame_timestamp_ns_, std::nullopt);
    if (!timestamp.has_value()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "process_input requires a timestamp from begin_frame"));
    }

    // Native input callbacks are frame-asynchronous. The reset guard clears
    // their accumulated events/deltas on every return or exception path.
    const TouchFrameReset reset{touch_input_};
    const TouchInputFrameView touch_frame{
        .events = std::span<const Input::PointerEvent>{
            touch_input_.frame_events()},
        .active_pointers = std::span<const Input::PointerSnapshot>{
            touch_input_.active_pointers()},
        .surface_generation = touch_input_.surface_generation(),
    };

    const Result<std::span<const Input::InputActionEvent>> actions =
        input_pipeline_.process_frame(touch_frame, *timestamp);
    if (!actions) {
        return Result<void>::failure(
            with_context(actions.error(), "process_frame"));
    }

    if (!actions.value().empty()) {
        const Result<void> consumed =
            action_consumer_.process_game_actions(actions.value());
        if (!consumed) {
            return Result<void>::failure(
                with_context(consumed.error(), "process_game_actions"));
        }
    }

    return delegate_.process_input();
}

Result<void> GestureActionGameLoopAdapter::process_events() {
    return delegate_.process_events();
}

Result<void> GestureActionGameLoopAdapter::fixed_update(
    std::chrono::nanoseconds fixed_delta) {
    return delegate_.fixed_update(fixed_delta);
}

Result<void> GestureActionGameLoopAdapter::variable_update(
    std::chrono::nanoseconds accepted_frame_delta) {
    return delegate_.variable_update(accepted_frame_delta);
}

Result<void> GestureActionGameLoopAdapter::render(double interpolation_alpha) {
    return delegate_.render(interpolation_alpha);
}

} // namespace Storm::Runtime
