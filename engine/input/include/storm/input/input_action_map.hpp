#pragma once

#include "storm/input/gesture_recognizer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Storm::Input {

using InputActionId = std::uint32_t;

inline constexpr InputActionId invalid_input_action_id = 0U;

enum class InputActionPhase : std::uint8_t {
    Started,
    Performed,
    Completed,
    Cancelled,
};

struct InputActionBinding final {
    GestureType gesture{GestureType::Tap};
    InputActionId action_id{invalid_input_action_id};
};

struct InputActionEvent final {
    InputActionId action_id{invalid_input_action_id};
    InputActionPhase phase{InputActionPhase::Performed};
    GestureEvent source{};
};

struct InputActionMapConfig final {
    std::size_t maximum_bindings{128U};
    std::size_t maximum_gesture_events_per_update{4096U};
    std::size_t maximum_action_events_per_update{256U};
};

// Maps recognized gestures to stable game-defined action IDs. Bindings are
// replaced atomically outside the per-frame update path. The map is
// single-thread-affine and performs no allocations while updating.
class InputActionMap final {
public:
    static constexpr std::size_t maximum_supported_bindings = 1024U;
    static constexpr std::size_t maximum_supported_gesture_events_per_update = 4096U;
    static constexpr std::size_t maximum_supported_action_events_per_update = 8192U;

    [[nodiscard]] static Result<InputActionMap> create(
        InputActionMapConfig config = {},
        std::span<const InputActionBinding> initial_bindings = {});

    InputActionMap(const InputActionMap&) = delete;
    InputActionMap& operator=(const InputActionMap&) = delete;
    InputActionMap(InputActionMap&&) noexcept = default;
    InputActionMap& operator=(InputActionMap&&) noexcept = default;

    // Duplicate (gesture, action_id) pairs and invalid identifiers are rejected.
    // A failed replacement leaves the current bindings unchanged.
    [[nodiscard]] Result<void> replace_bindings(
        std::span<const InputActionBinding> bindings);

    // Maps an ordered batch. The returned span is valid until update() or
    // replace_bindings() is called again. On any error, no partial output is
    // exposed.
    [[nodiscard]] Result<std::span<const InputActionEvent>> update(
        std::span<const GestureEvent> gestures);

    [[nodiscard]] std::span<const InputActionBinding> bindings() const noexcept;

private:
    explicit InputActionMap(InputActionMapConfig config);

    [[nodiscard]] static bool is_valid_gesture(GestureType gesture) noexcept;
    [[nodiscard]] static bool is_valid_gesture_event(
        const GestureEvent& gesture) noexcept;
    [[nodiscard]] static InputActionPhase phase_for(
        const GestureEvent& gesture) noexcept;

    InputActionMapConfig config_{};
    std::vector<InputActionBinding> bindings_;
    std::vector<InputActionEvent> output_;
    std::optional<std::uint64_t> last_timestamp_ns_;
};

} // namespace Storm::Input
