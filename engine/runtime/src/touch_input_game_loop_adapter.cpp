#include "storm/runtime/touch_input_game_loop_adapter.hpp"

namespace Storm::Runtime {

TouchInputGameLoopAdapter::TouchInputGameLoopAdapter(
    IGameLoopCallbacks& delegate,
    Input::TouchInput& touch_input,
    ITouchInputFrameConsumer& touch_consumer) noexcept
    : delegate_(delegate),
      touch_input_(touch_input),
      touch_consumer_(touch_consumer) {}

Result<void> TouchInputGameLoopAdapter::process_input() {
    const TouchInputFrameView frame{
        .events = std::span<const Input::PointerEvent>{touch_input_.frame_events()},
        .active_pointers =
            std::span<const Input::PointerSnapshot>{touch_input_.active_pointers()},
        .surface_generation = touch_input_.surface_generation(),
    };

    const Result<void> consumed = touch_consumer_.process_touch_input(frame);

    // Pointer events arrive asynchronously between game-loop ticks. Clear
    // them only after the consumer has observed the complete frame. Clearing
    // on failure also prevents replaying stale transitions on a later frame.
    touch_input_.begin_frame();

    if (!consumed) {
        return Result<void>::failure(consumed.error());
    }
    return delegate_.process_input();
}

Result<void> TouchInputGameLoopAdapter::process_events() {
    return delegate_.process_events();
}

Result<void> TouchInputGameLoopAdapter::fixed_update(
    std::chrono::nanoseconds fixed_delta) {
    return delegate_.fixed_update(fixed_delta);
}

Result<void> TouchInputGameLoopAdapter::variable_update(
    std::chrono::nanoseconds accepted_frame_delta) {
    return delegate_.variable_update(accepted_frame_delta);
}

Result<void> TouchInputGameLoopAdapter::render(double interpolation_alpha) {
    return delegate_.render(interpolation_alpha);
}

} // namespace Storm::Runtime
