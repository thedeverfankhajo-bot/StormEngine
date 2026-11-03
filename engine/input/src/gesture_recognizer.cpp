#include "storm/input/gesture_recognizer.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Input {
namespace {

constexpr double kMinimumPinchDistance = 1.0e-6;

[[nodiscard]] double distance(
    double first_x, double first_y, double second_x, double second_y) noexcept {
    return std::hypot(second_x - first_x, second_y - first_y);
}

} // namespace

GestureRecognizer::GestureRecognizer(GestureRecognizerConfig config)
    : config_(config) {
    output_.reserve(config_.maximum_gesture_events_per_frame);
}

Result<GestureRecognizer> GestureRecognizer::create(GestureRecognizerConfig config) {
    if (!std::isfinite(config.movement_threshold) ||
        config.movement_threshold <= 0.0 ||
        !std::isfinite(config.double_tap_distance) ||
        config.double_tap_distance <= 0.0 ||
        !std::isfinite(config.minimum_swipe_velocity) ||
        config.minimum_swipe_velocity <= 0.0 ||
        config.maximum_tap_duration_ns == 0U ||
        config.double_tap_interval_ns == 0U ||
        config.long_press_duration_ns == 0U ||
        config.maximum_gesture_events_per_frame == 0U ||
        config.maximum_gesture_events_per_frame >
            maximum_supported_gesture_events_per_frame) {
        return Result<GestureRecognizer>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "GestureRecognizer configuration contains an invalid threshold or capacity"));
    }

    try {
        return Result<GestureRecognizer>::success(GestureRecognizer{config});
    } catch (const std::bad_alloc&) {
        return Result<GestureRecognizer>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Unable to reserve GestureRecognizer output storage"));
    } catch (const std::length_error&) {
        return Result<GestureRecognizer>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "GestureRecognizer output capacity exceeds container limits"));
    }
}

GestureRecognizer::PointerTrack* GestureRecognizer::find_pointer(PointerId id) noexcept {
    const auto found = std::find_if(
        std::begin(pointers_), std::end(pointers_),
        [id](const PointerTrack& pointer) {
            return pointer.active && pointer.id == id;
        });
    return found == std::end(pointers_) ? nullptr : &*found;
}

GestureRecognizer::PointerTrack* GestureRecognizer::find_free_pointer() noexcept {
    const auto found = std::find_if(
        std::begin(pointers_), std::end(pointers_),
        [](const PointerTrack& pointer) { return !pointer.active; });
    return found == std::end(pointers_) ? nullptr : &*found;
}

bool GestureRecognizer::emit(const GestureEvent& event) noexcept {
    if (output_.size() >= config_.maximum_gesture_events_per_frame) {
        output_exhausted_ = true;
        return false;
    }
    output_.push_back(event);
    return true;
}

bool GestureRecognizer::begin_pinch_if_possible(std::uint64_t timestamp_ns) noexcept {
    if (pinch_.active) {
        return true;
    }

    PointerTrack* first = nullptr;
    PointerTrack* second = nullptr;
    for (PointerTrack& pointer : pointers_) {
        if (!pointer.active || pointer.participated_in_pinch) {
            continue;
        }
        if (first == nullptr) {
            first = &pointer;
        } else {
            second = &pointer;
            break;
        }
    }
    if (first == nullptr || second == nullptr) {
        return true;
    }

    const double initial_distance = distance(
        first->x, first->y, second->x, second->y);
    if (!std::isfinite(initial_distance)) {
        return false;
    }
    if (initial_distance <= kMinimumPinchDistance) {
        return true;
    }

    const double center_x = (first->x + second->x) * 0.5;
    const double center_y = (first->y + second->y) * 0.5;
    if (!std::isfinite(center_x) || !std::isfinite(center_y)) {
        return false;
    }

    pinch_ = PinchState{
        .active = true,
        .first = first->id,
        .second = second->id,
        .initial_distance = initial_distance,
        .last_center_x = center_x,
        .last_center_y = center_y,
    };
    first->participated_in_pinch = true;
    second->participated_in_pinch = true;

    const GestureEvent pan_begin{
        .type = GestureType::PanBegin,
        .pointer_id = first->id,
        .secondary_pointer_id = second->id,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
    };
    if (!emit(pan_begin)) {
        return false;
    }
    const GestureEvent pinch_begin{
        .type = GestureType::PinchBegin,
        .pointer_id = first->id,
        .secondary_pointer_id = second->id,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
        .scale = 1.0,
    };
    return emit(pinch_begin);
}

bool GestureRecognizer::update_pinch(std::uint64_t timestamp_ns) noexcept {
    if (!pinch_.active) {
        return true;
    }
    PointerTrack* first = find_pointer(pinch_.first);
    PointerTrack* second = find_pointer(pinch_.second);
    if (first == nullptr || second == nullptr) {
        return end_pinch(timestamp_ns, true);
    }

    const double current_distance = distance(
        first->x, first->y, second->x, second->y);
    const double center_x = (first->x + second->x) * 0.5;
    const double center_y = (first->y + second->y) * 0.5;
    if (!std::isfinite(current_distance) ||
        !std::isfinite(center_x) ||
        !std::isfinite(center_y)) {
        return false;
    }

    const double scale = current_distance / pinch_.initial_distance;
    const double delta_x = center_x - pinch_.last_center_x;
    const double delta_y = center_y - pinch_.last_center_y;
    if (!std::isfinite(scale) || !std::isfinite(delta_x) || !std::isfinite(delta_y)) {
        return false;
    }

    const GestureEvent pan_update{
        .type = GestureType::PanUpdate,
        .pointer_id = pinch_.first,
        .secondary_pointer_id = pinch_.second,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
        .delta_x = delta_x,
        .delta_y = delta_y,
    };
    if (!emit(pan_update)) {
        return false;
    }
    const GestureEvent pinch_update{
        .type = GestureType::PinchUpdate,
        .pointer_id = pinch_.first,
        .secondary_pointer_id = pinch_.second,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
        .scale = scale,
    };
    if (!emit(pinch_update)) {
        return false;
    }

    pinch_.last_center_x = center_x;
    pinch_.last_center_y = center_y;
    return true;
}

bool GestureRecognizer::end_pinch(
    std::uint64_t timestamp_ns, bool cancelled) noexcept {
    if (!pinch_.active) {
        return true;
    }
    PointerTrack* first = find_pointer(pinch_.first);
    PointerTrack* second = find_pointer(pinch_.second);
    double center_x = pinch_.last_center_x;
    double center_y = pinch_.last_center_y;
    double scale = 1.0;
    if (first != nullptr && second != nullptr) {
        const double current_distance = distance(
            first->x, first->y, second->x, second->y);
        center_x = (first->x + second->x) * 0.5;
        center_y = (first->y + second->y) * 0.5;
        scale = current_distance / pinch_.initial_distance;
        if (!std::isfinite(center_x) || !std::isfinite(center_y) ||
            !std::isfinite(scale)) {
            return false;
        }
    }

    const GestureEvent pinch_end{
        .type = GestureType::PinchEnd,
        .pointer_id = pinch_.first,
        .secondary_pointer_id = pinch_.second,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
        .scale = scale,
        .cancelled = cancelled,
    };
    if (!emit(pinch_end)) {
        return false;
    }
    const GestureEvent pan_end{
        .type = GestureType::PanEnd,
        .pointer_id = pinch_.first,
        .secondary_pointer_id = pinch_.second,
        .timestamp_ns = timestamp_ns,
        .x = center_x,
        .y = center_y,
        .cancelled = cancelled,
    };
    if (!emit(pan_end)) {
        return false;
    }

    pinch_ = {};
    return true;
}

bool GestureRecognizer::process_up(const PointerEvent& event) noexcept {
    PointerTrack* pointer = find_pointer(event.pointer_id);
    if (pointer == nullptr) {
        return false;
    }

    const double last_delta_x = event.x - pointer->x;
    const double last_delta_y = event.y - pointer->y;
    pointer->x = event.x;
    pointer->y = event.y;
    const double total_x = pointer->x - pointer->start_x;
    const double total_y = pointer->y - pointer->start_y;
    const double travelled = std::hypot(total_x, total_y);
    if (!std::isfinite(total_x) || !std::isfinite(total_y) ||
        !std::isfinite(travelled)) {
        return false;
    }

    if (pinch_.active &&
        (pinch_.first == event.pointer_id || pinch_.second == event.pointer_id)) {
        if (!update_pinch(event.timestamp_ns) ||
            !end_pinch(event.timestamp_ns, false)) {
            return false;
        }
    }

    const std::uint64_t duration_ns =
        event.timestamp_ns - pointer->down_timestamp_ns;
    if (!pointer->participated_in_pinch && !pointer->dragging &&
        !pointer->long_press_emitted &&
        travelled <= config_.movement_threshold &&
        duration_ns >= config_.long_press_duration_ns) {
        pointer->long_press_emitted = true;
        if (!emit(GestureEvent{
                .type = GestureType::LongPress,
                .pointer_id = pointer->id,
                .timestamp_ns = event.timestamp_ns,
                .x = pointer->x,
                .y = pointer->y,
            })) {
            return false;
        }
    }
    if (!pointer->participated_in_pinch) {
        if (!pointer->dragging && travelled > config_.movement_threshold) {
            pointer->dragging = true;
            if (!emit(GestureEvent{
                    .type = GestureType::DragBegin,
                    .pointer_id = pointer->id,
                    .timestamp_ns = event.timestamp_ns,
                    .x = pointer->start_x,
                    .y = pointer->start_y,
                })) {
                return false;
            }
        }

        if (pointer->dragging) {
            if (!emit(GestureEvent{
                    .type = GestureType::DragEnd,
                    .pointer_id = pointer->id,
                    .timestamp_ns = event.timestamp_ns,
                    .x = pointer->x,
                    .y = pointer->y,
                    .delta_x = last_delta_x,
                    .delta_y = last_delta_y,
                })) {
                return false;
            }
            if (duration_ns > 0U) {
                const double seconds = static_cast<double>(duration_ns) / 1.0e9;
                const double velocity_x = total_x / seconds;
                const double velocity_y = total_y / seconds;
                const double velocity = std::hypot(velocity_x, velocity_y);
                if (!std::isfinite(velocity_x) || !std::isfinite(velocity_y) ||
                    !std::isfinite(velocity)) {
                    return false;
                }
                if (velocity >= config_.minimum_swipe_velocity) {
                    if (!emit(GestureEvent{
                            .type = GestureType::Swipe,
                            .pointer_id = pointer->id,
                            .timestamp_ns = event.timestamp_ns,
                            .x = pointer->x,
                            .y = pointer->y,
                            .velocity_x = velocity_x,
                            .velocity_y = velocity_y,
                        })) {
                        return false;
                    }
                }
            }
        } else if (!pointer->long_press_emitted &&
                   travelled <= config_.movement_threshold &&
                   duration_ns <= config_.maximum_tap_duration_ns) {
            if (!emit(GestureEvent{
                    .type = GestureType::Tap,
                    .pointer_id = pointer->id,
                    .timestamp_ns = event.timestamp_ns,
                    .x = pointer->x,
                    .y = pointer->y,
                })) {
                return false;
            }

            if (has_last_tap_ &&
                event.timestamp_ns >= last_tap_timestamp_ns_ &&
                event.timestamp_ns - last_tap_timestamp_ns_ <=
                    config_.double_tap_interval_ns &&
                distance(last_tap_x_, last_tap_y_, pointer->x, pointer->y) <=
                    config_.double_tap_distance) {
                if (!emit(GestureEvent{
                        .type = GestureType::DoubleTap,
                        .pointer_id = pointer->id,
                        .timestamp_ns = event.timestamp_ns,
                        .x = pointer->x,
                        .y = pointer->y,
                    })) {
                    return false;
                }
                has_last_tap_ = false;
            } else {
                has_last_tap_ = true;
                last_tap_x_ = pointer->x;
                last_tap_y_ = pointer->y;
                last_tap_timestamp_ns_ = event.timestamp_ns;
            }
        }
    }

    pointer->active = false;
    return true;
}

bool GestureRecognizer::process_cancel(const PointerEvent& event) noexcept {
    PointerTrack* pointer = find_pointer(event.pointer_id);
    if (pointer == nullptr) {
        return false;
    }

    pointer->x = event.x;
    pointer->y = event.y;
    if (pinch_.active &&
        (pinch_.first == event.pointer_id || pinch_.second == event.pointer_id)) {
        if (!end_pinch(event.timestamp_ns, true)) {
            return false;
        }
    }
    if (pointer->dragging &&
        !emit(GestureEvent{
            .type = GestureType::DragEnd,
            .pointer_id = pointer->id,
            .timestamp_ns = event.timestamp_ns,
            .x = pointer->x,
            .y = pointer->y,
            .cancelled = true,
        })) {
        return false;
    }
    if (!emit(GestureEvent{
            .type = GestureType::Cancelled,
            .pointer_id = pointer->id,
            .timestamp_ns = event.timestamp_ns,
            .x = pointer->x,
            .y = pointer->y,
            .cancelled = true,
        })) {
        return false;
    }

    pointer->active = false;
    has_last_tap_ = false;
    return true;
}

bool GestureRecognizer::process_event(const PointerEvent& event) noexcept {
    switch (event.phase) {
        case PointerPhase::Down: {
            if (find_pointer(event.pointer_id) != nullptr) {
                return false;
            }
            PointerTrack* pointer = find_free_pointer();
            if (pointer == nullptr) {
                return false;
            }
            *pointer = PointerTrack{
                .active = true,
                .id = event.pointer_id,
                .start_x = event.x,
                .start_y = event.y,
                .x = event.x,
                .y = event.y,
                .down_timestamp_ns = event.timestamp_ns,
            };
            return begin_pinch_if_possible(event.timestamp_ns);
        }
        case PointerPhase::Move: {
            PointerTrack* pointer = find_pointer(event.pointer_id);
            if (pointer == nullptr) {
                return false;
            }
            const double delta_x = event.x - pointer->x;
            const double delta_y = event.y - pointer->y;
            const double total_distance = distance(
                pointer->start_x, pointer->start_y, event.x, event.y);
            if (!std::isfinite(delta_x) || !std::isfinite(delta_y) ||
                !std::isfinite(total_distance)) {
                return false;
            }

            pointer->x = event.x;
            pointer->y = event.y;

            if (!pointer->participated_in_pinch) {
                if (!pointer->dragging && !pointer->long_press_emitted &&
                    total_distance > config_.movement_threshold) {
                    pointer->dragging = true;
                    if (!emit(GestureEvent{
                            .type = GestureType::DragBegin,
                            .pointer_id = pointer->id,
                            .timestamp_ns = event.timestamp_ns,
                            .x = pointer->start_x,
                            .y = pointer->start_y,
                        })) {
                        return false;
                    }
                }
                if (pointer->dragging &&
                    !emit(GestureEvent{
                        .type = GestureType::DragUpdate,
                        .pointer_id = pointer->id,
                        .timestamp_ns = event.timestamp_ns,
                        .x = pointer->x,
                        .y = pointer->y,
                        .delta_x = delta_x,
                        .delta_y = delta_y,
                    })) {
                    return false;
                }
            }

            if (pinch_.active) {
                return update_pinch(event.timestamp_ns);
            }
            return begin_pinch_if_possible(event.timestamp_ns);
        }
        case PointerPhase::Up:
            return process_up(event);
        case PointerPhase::Cancel:
            return process_cancel(event);
        default:
            return false;
    }
}

bool GestureRecognizer::recognize_long_presses(
    std::uint64_t frame_timestamp_ns) noexcept {
    for (PointerTrack& pointer : pointers_) {
        if (!pointer.active || pointer.dragging ||
            pointer.long_press_emitted || pointer.participated_in_pinch ||
            frame_timestamp_ns < pointer.down_timestamp_ns) {
            continue;
        }
        const double travelled = distance(
            pointer.start_x, pointer.start_y, pointer.x, pointer.y);
        if (!std::isfinite(travelled)) {
            return false;
        }
        if (travelled > config_.movement_threshold ||
            frame_timestamp_ns - pointer.down_timestamp_ns <
                config_.long_press_duration_ns) {
            continue;
        }
        pointer.long_press_emitted = true;
        if (!emit(GestureEvent{
                .type = GestureType::LongPress,
                .pointer_id = pointer.id,
                .timestamp_ns = frame_timestamp_ns,
                .x = pointer.x,
                .y = pointer.y,
            })) {
            return false;
        }
    }
    return true;
}

bool GestureRecognizer::cancel_tracked_pointers(
    std::uint64_t timestamp_ns) noexcept {
    if (pinch_.active && !end_pinch(timestamp_ns, true)) {
        return false;
    }
    for (PointerTrack& pointer : pointers_) {
        if (!pointer.active) {
            continue;
        }
        if (!emit(GestureEvent{
                .type = GestureType::Cancelled,
                .pointer_id = pointer.id,
                .timestamp_ns = timestamp_ns,
                .x = pointer.x,
                .y = pointer.y,
                .cancelled = true,
            })) {
            return false;
        }
        pointer.active = false;
    }
    return true;
}

Result<std::span<const GestureEvent>> GestureRecognizer::update(
    std::span<const PointerEvent> pointer_events,
    std::uint64_t frame_timestamp_ns,
    std::optional<std::uint64_t> surface_generation) {
    output_.clear();
    output_exhausted_ = false;

    const auto fail_and_reset = [this](ErrorCode code, const char* message) {
        reset();
        return Result<std::span<const GestureEvent>>::failure(
            Error::make(code, message));
    };

    if (last_frame_timestamp_ns_.has_value() &&
        frame_timestamp_ns < *last_frame_timestamp_ns_) {
        return fail_and_reset(
            ErrorCode::InvalidArgument,
            "GestureRecognizer frame timestamp moved backwards");
    }
    if (surface_generation.has_value() && *surface_generation == 0U) {
        return fail_and_reset(
            ErrorCode::InvalidArgument,
            "GestureRecognizer surface generation must be non-zero");
    }

    if (!surface_state_initialized_ ||
        surface_generation_ != surface_generation) {
        if (!cancel_tracked_pointers(frame_timestamp_ns)) {
            return fail_and_reset(
                ErrorCode::ResourceExhausted,
                "GestureRecognizer output capacity was exhausted during surface cancellation");
        }
        has_last_tap_ = false;
        surface_generation_ = surface_generation;
        surface_state_initialized_ = true;
    }

    if (!pointer_events.empty() && !surface_generation.has_value()) {
        return fail_and_reset(
            ErrorCode::InvalidState,
            "Pointer events require an active surface generation");
    }

    std::optional<std::uint64_t> previous_event = last_event_timestamp_ns_;
    for (const PointerEvent& event : pointer_events) {
        if (!std::isfinite(event.x) || !std::isfinite(event.y)) {
            return fail_and_reset(
                ErrorCode::InvalidArgument,
                "GestureRecognizer pointer coordinates must be finite");
        }
        if (event.surface_generation != *surface_generation) {
            return fail_and_reset(
                ErrorCode::InvalidState,
                "GestureRecognizer rejected a stale surface generation");
        }
        if ((previous_event.has_value() &&
             event.timestamp_ns < *previous_event) ||
            event.timestamp_ns > frame_timestamp_ns) {
            return fail_and_reset(
                ErrorCode::InvalidArgument,
                "GestureRecognizer received an invalid pointer timestamp");
        }
        previous_event = event.timestamp_ns;
    }

    for (const PointerEvent& event : pointer_events) {
        if (!process_event(event)) {
            return fail_and_reset(
                output_exhausted_ ? ErrorCode::ResourceExhausted : ErrorCode::InvalidState,
                output_exhausted_
                    ? "GestureRecognizer output capacity was exhausted"
                    : "GestureRecognizer received an invalid pointer transition or numeric range");
        }
        last_event_timestamp_ns_ = event.timestamp_ns;
    }

    if (!recognize_long_presses(frame_timestamp_ns)) {
        return fail_and_reset(
            ErrorCode::ResourceExhausted,
            "GestureRecognizer output capacity was exhausted");
    }
    last_frame_timestamp_ns_ = frame_timestamp_ns;

    return Result<std::span<const GestureEvent>>::success(
        std::span<const GestureEvent>{output_});
}

void GestureRecognizer::reset() noexcept {
    output_.clear();
    output_exhausted_ = false;
    for (PointerTrack& pointer : pointers_) {
        pointer = {};
    }
    pinch_ = {};
    surface_generation_.reset();
    last_frame_timestamp_ns_.reset();
    last_event_timestamp_ns_.reset();
    surface_state_initialized_ = false;
    has_last_tap_ = false;
    last_tap_x_ = 0.0;
    last_tap_y_ = 0.0;
    last_tap_timestamp_ns_ = 0U;
}

} // namespace Storm::Input
