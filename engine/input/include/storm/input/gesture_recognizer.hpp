#pragma once

#include "storm/input/touch_input.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Storm::Input {

enum class GestureType : std::uint8_t {
    Tap,
    DoubleTap,
    LongPress,
    DragBegin,
    DragUpdate,
    DragEnd,
    Swipe,
    PanBegin,
    PanUpdate,
    PanEnd,
    PinchBegin,
    PinchUpdate,
    PinchEnd,
    Cancelled,
};

struct GestureEvent final {
    GestureType type{GestureType::Tap};
    PointerId pointer_id{0U};
    PointerId secondary_pointer_id{0U};
    std::uint64_t timestamp_ns{0U};
    double x{0.0};
    double y{0.0};
    double delta_x{0.0};
    double delta_y{0.0};
    double scale{1.0};
    double velocity_x{0.0};
    double velocity_y{0.0};
    bool cancelled{false};
};

struct GestureRecognizerConfig final {
    double movement_threshold{8.0};
    double double_tap_distance{24.0};
    double minimum_swipe_velocity{700.0};
    std::uint64_t maximum_tap_duration_ns{250'000'000U};
    std::uint64_t double_tap_interval_ns{350'000'000U};
    std::uint64_t long_press_duration_ns{500'000'000U};
    std::size_t maximum_gesture_events_per_frame{256U};
};

// Bounded, platform-neutral recognizer. It consumes the ordered pointer
// transitions exposed by TouchInput and owns no native handles or clock.
// The returned span is valid until the next update() or reset() call.
class GestureRecognizer final {
public:
    static constexpr std::size_t maximum_supported_pointers =
        TouchInput::maximum_supported_pointers;
    static constexpr std::size_t maximum_supported_gesture_events_per_frame = 4096U;

    [[nodiscard]] static Result<GestureRecognizer> create(
        GestureRecognizerConfig config = {});

    GestureRecognizer(const GestureRecognizer&) = delete;
    GestureRecognizer& operator=(const GestureRecognizer&) = delete;
    GestureRecognizer(GestureRecognizer&&) noexcept = default;
    GestureRecognizer& operator=(GestureRecognizer&&) noexcept = default;

    [[nodiscard]] Result<std::span<const GestureEvent>> update(
        std::span<const PointerEvent> pointer_events,
        std::uint64_t frame_timestamp_ns,
        std::optional<std::uint64_t> surface_generation);

    void reset() noexcept;

private:
    struct PointerTrack final {
        bool active{false};
        bool dragging{false};
        bool long_press_emitted{false};
        bool participated_in_pinch{false};
        PointerId id{0U};
        double start_x{0.0};
        double start_y{0.0};
        double x{0.0};
        double y{0.0};
        std::uint64_t down_timestamp_ns{0U};
    };

    struct PinchState final {
        bool active{false};
        PointerId first{0U};
        PointerId second{0U};
        double initial_distance{0.0};
        double last_center_x{0.0};
        double last_center_y{0.0};
    };

    explicit GestureRecognizer(GestureRecognizerConfig config);

    [[nodiscard]] PointerTrack* find_pointer(PointerId id) noexcept;
    [[nodiscard]] PointerTrack* find_free_pointer() noexcept;
    [[nodiscard]] bool emit(const GestureEvent& event) noexcept;
    [[nodiscard]] bool begin_pinch_if_possible(std::uint64_t timestamp_ns) noexcept;
    [[nodiscard]] bool update_pinch(std::uint64_t timestamp_ns) noexcept;
    [[nodiscard]] bool end_pinch(
        std::uint64_t timestamp_ns, bool cancelled) noexcept;
    [[nodiscard]] bool process_event(const PointerEvent& event) noexcept;
    [[nodiscard]] bool process_up(const PointerEvent& event) noexcept;
    [[nodiscard]] bool process_cancel(const PointerEvent& event) noexcept;
    [[nodiscard]] bool recognize_long_presses(std::uint64_t frame_timestamp_ns) noexcept;
    [[nodiscard]] bool cancel_tracked_pointers(std::uint64_t timestamp_ns) noexcept;

    GestureRecognizerConfig config_{};
    std::vector<GestureEvent> output_;
    std::optional<std::uint64_t> surface_generation_;
    std::optional<std::uint64_t> last_frame_timestamp_ns_;
    std::optional<std::uint64_t> last_event_timestamp_ns_;
    bool surface_state_initialized_{false};
    bool output_exhausted_{false};
    bool has_last_tap_{false};
    double last_tap_x_{0.0};
    double last_tap_y_{0.0};
    std::uint64_t last_tap_timestamp_ns_{0U};
    PinchState pinch_{};
    PointerTrack pointers_[maximum_supported_pointers]{};
};

} // namespace Storm::Input
