#include "storm/input/touch_input.hpp"
#include <algorithm>
#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Input {
TouchInput::TouchInput(TouchInputConfig config) noexcept : config_(config) {}

TouchInput::TouchInput(TouchInput&& other) noexcept
    : config_(other.config_),
      active_pointers_(std::move(other.active_pointers_)),
      frame_events_(std::move(other.frame_events_)),
      surface_generation_(std::exchange(other.surface_generation_, std::nullopt)),
      last_surface_generation_(std::exchange(other.last_surface_generation_, 0U)),
      last_event_timestamp_ns_(other.last_event_timestamp_ns_),
      has_last_event_timestamp_(other.has_last_event_timestamp_),
      initialized_(std::exchange(other.initialized_, false)) {
    other.last_event_timestamp_ns_ = 0U;
    other.has_last_event_timestamp_ = false;
}

Result<TouchInput> TouchInput::create(TouchInputConfig config) {
    if (config.maximum_active_pointers == 0U ||
        config.maximum_active_pointers > maximum_supported_pointers) {
        return Result<TouchInput>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "TouchInput pointer capacity must be between 1 and 64"));
    }
    if (config.maximum_events_per_frame < config.maximum_active_pointers ||
        config.maximum_events_per_frame > maximum_supported_events_per_frame) {
        return Result<TouchInput>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "TouchInput event capacity must cover cancellations and be at most 4096"));
    }
    try {
        TouchInput input(config);
        input.active_pointers_.reserve(config.maximum_active_pointers);
        input.frame_events_.reserve(config.maximum_events_per_frame);
        return Result<TouchInput>::success(std::move(input));
    } catch (const std::bad_alloc&) {
        return Result<TouchInput>::failure(Error::make(
            ErrorCode::ResourceExhausted, "Unable to reserve TouchInput buffers"));
    } catch (const std::length_error&) {
        return Result<TouchInput>::failure(Error::make(
            ErrorCode::ResourceExhausted, "TouchInput capacity exceeds container limits"));
    }
}

Result<void> TouchInput::ensure_initialized() const {
    if (!initialized_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "TouchInput instance is moved-from"));
    }
    return Result<void>::success();
}

Result<void> TouchInput::attach_surface(std::uint64_t generation) {
    auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (generation == 0U || generation <= last_surface_generation_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument, "Surface generation must increase monotonically"));
    }
    if (surface_generation_.has_value() || !active_pointers_.empty()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "A surface is already attached or pointers remain active"));
    }
    surface_generation_ = generation;
    last_surface_generation_ = generation;
    return Result<void>::success();
}

Result<void> TouchInput::detach_surface(
    std::uint64_t generation, std::uint64_t timestamp_ns) {
    auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (!surface_generation_.has_value() || *surface_generation_ != generation) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Cannot detach a stale or unknown surface generation"));
    }
    auto cancelled = cancel_all(timestamp_ns);
    if (!cancelled) return cancelled;
    surface_generation_.reset();
    return Result<void>::success();
}

std::vector<PointerSnapshot>::iterator TouchInput::find_pointer(PointerId id) noexcept {
    return std::find_if(active_pointers_.begin(), active_pointers_.end(),
        [id](const PointerSnapshot& p) { return p.pointer_id == id; });
}

Result<void> TouchInput::process(const PointerEvent& event) {
    auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (!surface_generation_.has_value()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Pointer events require an attached surface"));
    }
    if (event.surface_generation != *surface_generation_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Pointer event belongs to a stale surface"));
    }
    if (!std::isfinite(event.x) || !std::isfinite(event.y)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument, "Pointer coordinates must be finite"));
    }
    if (frame_events_.size() >= config_.maximum_events_per_frame) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted, "TouchInput per-frame event capacity is exhausted"));
    }

    switch (event.phase) {
        case PointerPhase::Down: {
            if (find_pointer(event.pointer_id) != active_pointers_.end()) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidState, "Pointer is already active"));
            }
            if (active_pointers_.size() >= config_.maximum_active_pointers) {
                return Result<void>::failure(Error::make(
                    ErrorCode::ResourceExhausted, "TouchInput active-pointer capacity is exhausted"));
            }
            if (has_last_event_timestamp_ && event.timestamp_ns < last_event_timestamp_ns_) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument, "Pointer timestamp moved backwards"));
            }
            active_pointers_.push_back(PointerSnapshot{
                event.pointer_id, event.x, event.y, 0.0, 0.0, event.timestamp_ns});
            frame_events_.push_back(event);
            last_event_timestamp_ns_ = event.timestamp_ns;
            has_last_event_timestamp_ = true;
            return Result<void>::success();
        }
        case PointerPhase::Move:
        case PointerPhase::Up:
        case PointerPhase::Cancel: {
            const auto pointer = find_pointer(event.pointer_id);
            if (pointer == active_pointers_.end()) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidState, "Pointer transition requires an active pointer"));
            }
            if (has_last_event_timestamp_ && event.timestamp_ns < last_event_timestamp_ns_) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument, "Pointer timestamp moved backwards"));
            }
            const double dx = event.x - pointer->x;
            const double dy = event.y - pointer->y;
            if (!std::isfinite(dx) || !std::isfinite(dy)) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument, "Pointer delta is outside the supported numeric range"));
            }
            if (event.phase == PointerPhase::Move &&
                (!std::isfinite(pointer->delta_x + dx) ||
                 !std::isfinite(pointer->delta_y + dy))) {
                return Result<void>::failure(Error::make(
                    ErrorCode::InvalidArgument, "Accumulated pointer delta is not finite"));
            }
            frame_events_.push_back(event);
            last_event_timestamp_ns_ = event.timestamp_ns;
            has_last_event_timestamp_ = true;
            if (event.phase == PointerPhase::Move) {
                pointer->x = event.x;
                pointer->y = event.y;
                pointer->delta_x += dx;
                pointer->delta_y += dy;
                pointer->timestamp_ns = event.timestamp_ns;
            } else {
                active_pointers_.erase(pointer);
            }
            return Result<void>::success();
        }
        default:
            return Result<void>::failure(Error::make(
                ErrorCode::InvalidArgument, "Unknown pointer phase"));
    }
}

Result<void> TouchInput::process_batch(std::span<const PointerEvent> events) {
    auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (events.empty()) return Result<void>::success();

    for (const PointerEvent& event : events) {
        auto result = process(event);
        if (result) continue;

        const Error original_error = result.error();
        // Native multi-pointer callbacks are one logical batch. If a later
        // pointer is invalid, never leave a partially applied gesture active.
        begin_frame();
        if (!active_pointers_.empty()) {
            auto cancelled = cancel_all(last_event_timestamp_ns_);
            if (!cancelled) {
                return Result<void>::failure(Error::make(
                    ErrorCode::Internal,
                    "TouchInput batch failed and active-pointer cleanup also failed: " +
                        cancelled.error().message));
            }
        }
        return Result<void>::failure(original_error);
    }
    return Result<void>::success();
}

Result<void> TouchInput::cancel_all(std::uint64_t timestamp_ns) {
    auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (active_pointers_.empty()) return Result<void>::success();
    if (!surface_generation_.has_value()) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Active pointers cannot be cancelled without a surface"));
    }
    if (has_last_event_timestamp_ && timestamp_ns < last_event_timestamp_ns_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument, "Cancellation timestamp moved backwards"));
    }
    if (active_pointers_.size() >
        config_.maximum_events_per_frame - frame_events_.size()) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted, "Insufficient capacity to report pointer cancellations"));
    }
    for (const PointerSnapshot& pointer : active_pointers_) {
        frame_events_.push_back(PointerEvent{
            pointer.pointer_id, PointerPhase::Cancel, pointer.x, pointer.y,
            timestamp_ns, *surface_generation_});
    }
    active_pointers_.clear();
    last_event_timestamp_ns_ = timestamp_ns;
    has_last_event_timestamp_ = true;
    return Result<void>::success();
}

void TouchInput::begin_frame() noexcept {
    if (!initialized_) return;
    frame_events_.clear();
    for (PointerSnapshot& pointer : active_pointers_) {
        pointer.delta_x = 0.0;
        pointer.delta_y = 0.0;
    }
}
const std::vector<PointerSnapshot>& TouchInput::active_pointers() const noexcept {
    return active_pointers_;
}
const std::vector<PointerEvent>& TouchInput::frame_events() const noexcept {
    return frame_events_;
}
std::optional<std::uint64_t> TouchInput::surface_generation() const noexcept {
    return surface_generation_;
}
std::size_t TouchInput::active_pointer_count() const noexcept {
    return active_pointers_.size();
}
} // namespace Storm::Input
