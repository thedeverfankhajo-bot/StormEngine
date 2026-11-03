#pragma once

#include "storm/core/game_loop.hpp"
#include "storm/input/touch_input.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>

namespace Storm::Runtime {

// A non-owning view valid only for the duration of
// ITouchInputFrameConsumer::process_touch_input().
struct TouchInputFrameView final {
    std::span<const Input::PointerEvent> events;
    std::span<const Input::PointerSnapshot> active_pointers;
    std::optional<std::uint64_t> surface_generation;
};

class ITouchInputFrameConsumer {
public:
    virtual ~ITouchInputFrameConsumer() = default;

    [[nodiscard]] virtual Result<void> process_touch_input(
        const TouchInputFrameView& frame) = 0;
};

// Decorates application callbacks with a frame-scoped TouchInput consumer.
// Native callbacks may accumulate events between display frames. The adapter
// presents those events during GameLoop::process_input(), then clears the
// per-frame event/delta data even when the consumer reports an error.
class TouchInputGameLoopAdapter final : public IGameLoopCallbacks {
public:
    TouchInputGameLoopAdapter(
        IGameLoopCallbacks& delegate,
        Input::TouchInput& touch_input,
        ITouchInputFrameConsumer& touch_consumer) noexcept;

    TouchInputGameLoopAdapter(const TouchInputGameLoopAdapter&) = delete;
    TouchInputGameLoopAdapter& operator=(const TouchInputGameLoopAdapter&) = delete;
    TouchInputGameLoopAdapter(TouchInputGameLoopAdapter&&) = delete;
    TouchInputGameLoopAdapter& operator=(TouchInputGameLoopAdapter&&) = delete;

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
    ITouchInputFrameConsumer& touch_consumer_;
};

} // namespace Storm::Runtime
