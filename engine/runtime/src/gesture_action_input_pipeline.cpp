#include "storm/runtime/gesture_action_input_pipeline.hpp"

#include <utility>

namespace Storm::Runtime {

GestureActionInputPipeline::GestureActionInputPipeline(
    Input::GestureRecognizer recognizer,
    Input::InputActionMap action_map) noexcept
    : recognizer_(std::move(recognizer)),
      action_map_(std::move(action_map)) {}

Result<GestureActionInputPipeline> GestureActionInputPipeline::create(
    GestureActionInputPipelineConfig config,
    std::span<const Input::InputActionBinding> initial_bindings) {
    auto recognizer = Input::GestureRecognizer::create(config.gesture);
    if (!recognizer) {
        return Result<GestureActionInputPipeline>::failure(recognizer.error());
    }

    auto action_map = Input::InputActionMap::create(config.actions, initial_bindings);
    if (!action_map) {
        return Result<GestureActionInputPipeline>::failure(action_map.error());
    }

    return Result<GestureActionInputPipeline>::success(
        GestureActionInputPipeline{
            std::move(recognizer).value(),
            std::move(action_map).value(),
        });
}

Result<std::span<const Input::InputActionEvent>>
GestureActionInputPipeline::process_frame(
    const TouchInputFrameView& touch_frame,
    std::uint64_t frame_timestamp_ns) {
    const auto gestures = recognizer_.update(
        touch_frame.events,
        frame_timestamp_ns,
        touch_frame.surface_generation);
    if (!gestures) {
        return Result<std::span<const Input::InputActionEvent>>::failure(
            gestures.error());
    }

    return action_map_.update(gestures.value());
}

Result<void> GestureActionInputPipeline::replace_bindings(
    std::span<const Input::InputActionBinding> bindings) {
    return action_map_.replace_bindings(bindings);
}

std::span<const Input::InputActionBinding>
GestureActionInputPipeline::bindings() const noexcept {
    return action_map_.bindings();
}

} // namespace Storm::Runtime
