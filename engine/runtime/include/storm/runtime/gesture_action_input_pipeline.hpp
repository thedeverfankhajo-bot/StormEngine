#pragma once

#include "storm/input/input_action_map.hpp"
#include "storm/runtime/touch_input_game_loop_adapter.hpp"

#include <cstdint>
#include <span>

namespace Storm::Runtime {

struct GestureActionInputPipelineConfig final {
    Input::GestureRecognizerConfig gesture{};
    Input::InputActionMapConfig actions{};
};

// Converts frame-scoped native touch events into stable game actions.
// The caller supplies a monotonic frame timestamp in the same clock domain
// used by PointerEvent::timestamp_ns. This is required for time-based
// gestures such as LongPress; this class deliberately does not read a clock.
class GestureActionInputPipeline final {
public:
    [[nodiscard]] static Result<GestureActionInputPipeline> create(
        GestureActionInputPipelineConfig config = {},
        std::span<const Input::InputActionBinding> initial_bindings = {});

    GestureActionInputPipeline(const GestureActionInputPipeline&) = delete;
    GestureActionInputPipeline& operator=(const GestureActionInputPipeline&) = delete;
    GestureActionInputPipeline(GestureActionInputPipeline&&) noexcept = default;
    GestureActionInputPipeline& operator=(GestureActionInputPipeline&&) noexcept = default;

    // The returned action span is owned by this pipeline and remains valid
    // until the next process_frame() call or a successful binding replacement.
    [[nodiscard]] Result<std::span<const Input::InputActionEvent>> process_frame(
        const TouchInputFrameView& touch_frame,
        std::uint64_t frame_timestamp_ns);

    // Rebinding is validated atomically by InputActionMap. It is an explicit
    // configuration operation and may allocate; process_frame() does not.
    [[nodiscard]] Result<void> replace_bindings(
        std::span<const Input::InputActionBinding> bindings);

    [[nodiscard]] std::span<const Input::InputActionBinding> bindings() const noexcept;

private:
    GestureActionInputPipeline(
        Input::GestureRecognizer recognizer,
        Input::InputActionMap action_map) noexcept;

    Input::GestureRecognizer recognizer_;
    Input::InputActionMap action_map_;
};

} // namespace Storm::Runtime
