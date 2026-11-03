#include "storm/input/input_action_map.hpp"

#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Input {

InputActionMap::InputActionMap(InputActionMapConfig config)
    : config_(config) {
    bindings_.reserve(config_.maximum_bindings);
    output_.reserve(config_.maximum_action_events_per_update);
}

Result<InputActionMap> InputActionMap::create(
    InputActionMapConfig config,
    std::span<const InputActionBinding> initial_bindings) {
    if (config.maximum_bindings == 0U ||
        config.maximum_bindings > maximum_supported_bindings ||
        config.maximum_gesture_events_per_update == 0U ||
        config.maximum_gesture_events_per_update >
            maximum_supported_gesture_events_per_update ||
        config.maximum_action_events_per_update == 0U ||
        config.maximum_action_events_per_update >
            maximum_supported_action_events_per_update ||
        initial_bindings.size() > config.maximum_bindings) {
        return Result<InputActionMap>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "InputActionMap configuration or initial binding count is invalid"));
    }

    try {
        InputActionMap map{config};
        const Result<void> replaced = map.replace_bindings(initial_bindings);
        if (!replaced) {
            return Result<InputActionMap>::failure(replaced.error());
        }
        return Result<InputActionMap>::success(std::move(map));
    } catch (const std::bad_alloc&) {
        return Result<InputActionMap>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Unable to reserve InputActionMap storage"));
    } catch (const std::length_error&) {
        return Result<InputActionMap>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "InputActionMap capacity exceeds container limits"));
    }
}

bool InputActionMap::is_valid_gesture(GestureType gesture) noexcept {
    switch (gesture) {
        case GestureType::Tap:
        case GestureType::DoubleTap:
        case GestureType::LongPress:
        case GestureType::DragBegin:
        case GestureType::DragUpdate:
        case GestureType::DragEnd:
        case GestureType::Swipe:
        case GestureType::PanBegin:
        case GestureType::PanUpdate:
        case GestureType::PanEnd:
        case GestureType::PinchBegin:
        case GestureType::PinchUpdate:
        case GestureType::PinchEnd:
        case GestureType::Cancelled:
            return true;
        default:
            return false;
    }
}

bool InputActionMap::is_valid_gesture_event(
    const GestureEvent& gesture) noexcept {
    if (!is_valid_gesture(gesture.type) ||
        !std::isfinite(gesture.x) ||
        !std::isfinite(gesture.y) ||
        !std::isfinite(gesture.delta_x) ||
        !std::isfinite(gesture.delta_y) ||
        !std::isfinite(gesture.scale) ||
        gesture.scale <= 0.0 ||
        !std::isfinite(gesture.velocity_x) ||
        !std::isfinite(gesture.velocity_y)) {
        return false;
    }
    const bool is_terminal_gesture =
        gesture.type == GestureType::DragEnd ||
        gesture.type == GestureType::PanEnd ||
        gesture.type == GestureType::PinchEnd ||
        gesture.type == GestureType::Cancelled;
    if ((gesture.type == GestureType::Cancelled && !gesture.cancelled) ||
        (gesture.cancelled && !is_terminal_gesture)) {
        return false;
    }
    return true;
}

InputActionPhase InputActionMap::phase_for(
    const GestureEvent& gesture) noexcept {
    switch (gesture.type) {
        case GestureType::DragBegin:
        case GestureType::PanBegin:
        case GestureType::PinchBegin:
            return InputActionPhase::Started;

        case GestureType::DragEnd:
        case GestureType::PanEnd:
        case GestureType::PinchEnd:
            return gesture.cancelled
                ? InputActionPhase::Cancelled
                : InputActionPhase::Completed;

        case GestureType::Cancelled:
            return InputActionPhase::Cancelled;

        case GestureType::Tap:
        case GestureType::DoubleTap:
        case GestureType::LongPress:
        case GestureType::DragUpdate:
        case GestureType::Swipe:
        case GestureType::PanUpdate:
        case GestureType::PinchUpdate:
        default:
            return InputActionPhase::Performed;
    }
}

Result<void> InputActionMap::replace_bindings(
    std::span<const InputActionBinding> bindings) {
    if (bindings.size() > config_.maximum_bindings) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "InputActionMap binding capacity is exhausted"));
    }

    for (std::size_t index = 0U; index < bindings.size(); ++index) {
        const InputActionBinding& binding = bindings[index];
        if (binding.action_id == invalid_input_action_id ||
            !is_valid_gesture(binding.gesture)) {
            return Result<void>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "InputActionMap binding contains an invalid gesture or action ID"));
        }

        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (bindings[previous].gesture == binding.gesture &&
                bindings[previous].action_id == binding.action_id) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument,
                    "InputActionMap contains a duplicate gesture/action binding"));
            }
        }
    }

    try {
        std::vector<InputActionBinding> replacement;
        replacement.reserve(config_.maximum_bindings);
        replacement.assign(bindings.begin(), bindings.end());
        bindings_.swap(replacement);
        output_.clear();
        return Result<void>::success();
    } catch (const std::bad_alloc&) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Unable to allocate replacement InputActionMap bindings"));
    } catch (const std::length_error&) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Replacement InputActionMap binding capacity is invalid"));
    }
}

Result<std::span<const InputActionEvent>> InputActionMap::update(
    std::span<const GestureEvent> gestures) {
    output_.clear();

    if (gestures.size() > config_.maximum_gesture_events_per_update) {
        return Result<std::span<const InputActionEvent>>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "InputActionMap gesture batch exceeds the configured limit"));
    }

    std::optional<std::uint64_t> previous_timestamp = last_timestamp_ns_;
    for (const GestureEvent& gesture : gestures) {
        if (!is_valid_gesture_event(gesture)) {
            return Result<std::span<const InputActionEvent>>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "InputActionMap rejected an invalid gesture event"));
        }
        if (previous_timestamp.has_value() &&
            gesture.timestamp_ns < *previous_timestamp) {
            return Result<std::span<const InputActionEvent>>::failure(Error::make(
                ErrorCode::InvalidArgument,
                "InputActionMap rejected a non-monotonic gesture timestamp"));
        }
        previous_timestamp = gesture.timestamp_ns;
    }

    for (const GestureEvent& gesture : gestures) {
        for (const InputActionBinding& binding : bindings_) {
            if (binding.gesture != gesture.type) {
                continue;
            }
            if (output_.size() >= config_.maximum_action_events_per_update) {
                output_.clear();
                return Result<std::span<const InputActionEvent>>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "InputActionMap action-event output capacity is exhausted"));
            }
            output_.push_back(InputActionEvent{
                .action_id = binding.action_id,
                .phase = phase_for(gesture),
                .source = gesture,
            });
        }
    }

    if (previous_timestamp.has_value()) {
        last_timestamp_ns_ = previous_timestamp;
    }

    return Result<std::span<const InputActionEvent>>::success(
        std::span<const InputActionEvent>{output_});
}

std::span<const InputActionBinding> InputActionMap::bindings() const noexcept {
    return std::span<const InputActionBinding>{bindings_};
}

} // namespace Storm::Input
