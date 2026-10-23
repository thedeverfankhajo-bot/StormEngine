#pragma once

#include "storm/platform/input_state.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <variant>

namespace storm::platform {

// Native adapters translate OS callbacks into these small, engine-owned event values. Key slots
// deliberately use the InputState slot contract; platform adapters own the OS-to-slot mapping.
struct QuitRequestedEvent final {};

struct WindowFocusChangedEvent final {
    bool focused{};
};

struct KeyStateChangedEvent final {
    std::uint16_t key_code{};
    bool is_down{};
};

struct PointerButtonStateChangedEvent final {
    std::uint16_t button{};
    bool is_down{};
};

// Pointer coordinates are in the platform adapter's documented window-coordinate space.
struct PointerMovedEvent final {
    double x{};
    double y{};
};

struct PointerWheelScrolledEvent final {
    double delta_x{};
    double delta_y{};
};

// A zero width or height is valid when a window is minimized or its surface is unavailable.
struct WindowResizedEvent final {
    std::uint32_t width{};
    std::uint32_t height{};
};

using PlatformEvent = std::variant<QuitRequestedEvent, WindowFocusChangedEvent,
                                   KeyStateChangedEvent, PointerButtonStateChangedEvent,
                                   PointerMovedEvent, PointerWheelScrolledEvent,
                                   WindowResizedEvent>;

static_assert(std::is_nothrow_copy_constructible_v<PlatformEvent>);
static_assert(std::is_nothrow_copy_assignable_v<PlatformEvent>);

// Validates payloads before they enter the queue. Pointer/key slots match InputState exactly;
// floating-point coordinates must be finite, and minimized window dimensions are allowed.
[[nodiscard]] inline bool is_valid_platform_event(const PlatformEvent& event) noexcept {
    return std::visit(
        [](const auto& payload) noexcept {
            using Event = std::remove_cvref_t<decltype(payload)>;
            if constexpr (std::is_same_v<Event, KeyStateChangedEvent>) {
                return static_cast<std::size_t>(payload.key_code) < InputState::key_code_count;
            } else if constexpr (std::is_same_v<Event, PointerButtonStateChangedEvent>) {
                return static_cast<std::size_t>(payload.button) <
                       InputState::pointer_button_count;
            } else if constexpr (std::is_same_v<Event, PointerMovedEvent>) {
                return std::isfinite(payload.x) && std::isfinite(payload.y);
            } else if constexpr (std::is_same_v<Event, PointerWheelScrolledEvent>) {
                return std::isfinite(payload.delta_x) && std::isfinite(payload.delta_y);
            } else {
                return true;
            }
        },
        event);
}

// Fixed-capacity FIFO queue for native-platform events. It performs no heap allocation, never
// overwrites an accepted event, and exposes overflow/rejection counters instead of failing silently.
// The queue is single-owner-thread only; it is not a thread-safe transport between OS threads.
template <std::size_t Capacity = 256U>
class PlatformEventQueue final {
    static_assert(Capacity > 0U, "PlatformEventQueue capacity must be greater than zero");

public:
    static constexpr std::size_t capacity = Capacity;

    // Returns false and increments rejected_count() for malformed payloads.
    // Returns false and increments dropped_count() when full; existing queued events are preserved.
    [[nodiscard]] bool push(const PlatformEvent& event) noexcept {
        if (!is_valid_platform_event(event)) {
            saturating_increment(rejected_count_);
            return false;
        }
        if (size_ == Capacity) {
            saturating_increment(dropped_count_);
            return false;
        }

        const std::size_t tail = (head_ + size_) % Capacity;
        events_[tail] = event;
        ++size_;
        return true;
    }

    [[nodiscard]] std::optional<PlatformEvent> pop() noexcept {
        if (size_ == 0U) {
            return std::nullopt;
        }

        PlatformEvent event = events_[head_];
        events_[head_] = QuitRequestedEvent{};
        head_ = (head_ + 1U) % Capacity;
        --size_;
        return event;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept {
        return Capacity;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0U;
    }

    [[nodiscard]] std::uint64_t dropped_count() const noexcept {
        return dropped_count_;
    }

    [[nodiscard]] std::uint64_t rejected_count() const noexcept {
        return rejected_count_;
    }

    // Discards queued events and resets statistics. Native adapters should handle pending critical
    // state (especially input release on focus loss) before clearing the queue.
    void clear() noexcept {
        events_.fill(PlatformEvent{QuitRequestedEvent{}});
        head_ = 0U;
        size_ = 0U;
        dropped_count_ = 0U;
        rejected_count_ = 0U;
    }

private:
    static void saturating_increment(std::uint64_t& value) noexcept {
        if (value < std::numeric_limits<std::uint64_t>::max()) {
            ++value;
        }
    }

    std::array<PlatformEvent, Capacity> events_{};
    std::size_t head_{};
    std::size_t size_{};
    std::uint64_t dropped_count_{};
    std::uint64_t rejected_count_{};
};

} // namespace storm::platform
