#pragma once
#include "storm/foundation/result.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Storm::Input {
using PointerId = std::uint64_t;

enum class PointerPhase : std::uint8_t { Down, Move, Up, Cancel };

// Coordinates are logical points. Native adapters provide monotonic time
// and the active surface generation.
struct PointerEvent final {
    PointerId pointer_id{0U};
    PointerPhase phase{PointerPhase::Down};
    double x{0.0};
    double y{0.0};
    std::uint64_t timestamp_ns{0U};
    std::uint64_t surface_generation{0U};
};

struct PointerSnapshot final {
    PointerId pointer_id{0U};
    double x{0.0};
    double y{0.0};
    double delta_x{0.0};
    double delta_y{0.0};
    std::uint64_t timestamp_ns{0U};
};

struct TouchInputConfig final {
    std::size_t maximum_active_pointers{10U};
    std::size_t maximum_events_per_frame{256U};
};

// Bounded, single-thread-affine ingestion of multi-pointer events. This
// portable layer owns no OS handles and performs no gesture recognition.
class TouchInput final {
public:
    static constexpr std::size_t maximum_supported_pointers = 64U;
    static constexpr std::size_t maximum_supported_events_per_frame = 4096U;

    [[nodiscard]] static Result<TouchInput> create(TouchInputConfig config = {});
    TouchInput(const TouchInput&) = delete;
    TouchInput& operator=(const TouchInput&) = delete;
    TouchInput& operator=(TouchInput&&) = delete;
    TouchInput(TouchInput&& other) noexcept;

    [[nodiscard]] Result<void> attach_surface(std::uint64_t generation);
    [[nodiscard]] Result<void> detach_surface(
        std::uint64_t generation, std::uint64_t timestamp_ns);
    [[nodiscard]] Result<void> process(const PointerEvent& event);
    // On a failed transition, partial events are discarded and active pointers cancelled.
    [[nodiscard]] Result<void> process_batch(std::span<const PointerEvent> events);
    [[nodiscard]] Result<void> cancel_all(std::uint64_t timestamp_ns);

    // Call once at the start of each input frame.
    void begin_frame() noexcept;

    [[nodiscard]] const std::vector<PointerSnapshot>& active_pointers() const noexcept;
    [[nodiscard]] const std::vector<PointerEvent>& frame_events() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> surface_generation() const noexcept;
    [[nodiscard]] std::size_t active_pointer_count() const noexcept;

private:
    explicit TouchInput(TouchInputConfig config) noexcept;
    [[nodiscard]] std::vector<PointerSnapshot>::iterator find_pointer(PointerId id) noexcept;
    [[nodiscard]] Result<void> ensure_initialized() const;

    TouchInputConfig config_{};
    std::vector<PointerSnapshot> active_pointers_;
    std::vector<PointerEvent> frame_events_;
    std::optional<std::uint64_t> surface_generation_;
    std::uint64_t last_surface_generation_{0U};
    std::uint64_t last_event_timestamp_ns_{0U};
    bool has_last_event_timestamp_{false};
    bool initialized_{true};
};
} // namespace Storm::Input
