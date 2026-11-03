#include "storm/input/input_action_map.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {
using Storm::Input::GestureEvent;
using Storm::Input::GestureType;
using Storm::Input::InputActionBinding;
using Storm::Input::InputActionId;
using Storm::Input::InputActionMap;
using Storm::Input::InputActionMapConfig;
using Storm::Input::InputActionPhase;
using Storm::Input::PointerEvent;

#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

GestureEvent gesture(
    GestureType type,
    std::uint64_t timestamp,
    std::uint64_t pointer_id = 1U,
    bool cancelled = false) {
    return GestureEvent{
        .type = type,
        .pointer_id = pointer_id,
        .timestamp_ns = timestamp,
        .scale = 1.0,
        .cancelled = cancelled,
    };
}

std::optional<std::string> configuration_and_binding_validation() {
    EXPECT(!InputActionMap::create(InputActionMapConfig{.maximum_bindings = 0U}));
    EXPECT(!InputActionMap::create(InputActionMapConfig{
        .maximum_action_events_per_update =
            InputActionMap::maximum_supported_action_events_per_update + 1U}));
    EXPECT(!InputActionMap::create(
        InputActionMapConfig{},
        std::array{InputActionBinding{GestureType::Tap, 0U}}));
    EXPECT(!InputActionMap::create(
        InputActionMapConfig{},
        std::array{
            InputActionBinding{GestureType::Tap, 10U},
            InputActionBinding{GestureType::Tap, 10U},
        }));
    auto created = InputActionMap::create();
    EXPECT(created);
    EXPECT(created.value().bindings().empty());
    return std::nullopt;
}

std::optional<std::string> maps_gesture_lifecycle_to_action_phases() {
    const std::array bindings{
        InputActionBinding{GestureType::DragBegin, 100U},
        InputActionBinding{GestureType::DragUpdate, 100U},
        InputActionBinding{GestureType::DragEnd, 100U},
        InputActionBinding{GestureType::PinchBegin, 200U},
        InputActionBinding{GestureType::PinchUpdate, 200U},
        InputActionBinding{GestureType::PinchEnd, 200U},
    };
    auto created = InputActionMap::create(InputActionMapConfig{}, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    const std::array events{
        gesture(GestureType::DragBegin, 1U),
        gesture(GestureType::DragUpdate, 2U),
        gesture(GestureType::DragEnd, 3U),
        gesture(GestureType::PinchBegin, 4U),
        gesture(GestureType::PinchUpdate, 5U),
        gesture(GestureType::PinchEnd, 6U),
    };
    auto actions = map.update(events);
    EXPECT(actions);
    EXPECT(actions.value().size() == 6U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Started);
    EXPECT(actions.value()[1].phase == InputActionPhase::Performed);
    EXPECT(actions.value()[2].phase == InputActionPhase::Completed);
    EXPECT(actions.value()[3].phase == InputActionPhase::Started);
    EXPECT(actions.value()[4].phase == InputActionPhase::Performed);
    EXPECT(actions.value()[5].phase == InputActionPhase::Completed);
    EXPECT(actions.value()[2].source.type == GestureType::DragEnd);
    EXPECT(actions.value()[5].source.type == GestureType::PinchEnd);
    return std::nullopt;
}

std::optional<std::string> one_gesture_can_trigger_multiple_actions_in_binding_order() {
    const std::array bindings{
        InputActionBinding{GestureType::Tap, 7U},
        InputActionBinding{GestureType::Tap, 3U},
        InputActionBinding{GestureType::DoubleTap, 9U},
    };
    auto created = InputActionMap::create(InputActionMapConfig{}, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    const GestureEvent tap = gesture(GestureType::Tap, 20U, 5U);
    auto actions = map.update(std::span<const GestureEvent>{&tap, 1U});
    EXPECT(actions);
    EXPECT(actions.value().size() == 2U);
    EXPECT(actions.value()[0].action_id == 7U);
    EXPECT(actions.value()[1].action_id == 3U);
    EXPECT(actions.value()[0].source.pointer_id == 5U);
    return std::nullopt;
}

std::optional<std::string> cancelled_gestures_produce_cancelled_actions() {
    const std::array bindings{
        InputActionBinding{GestureType::DragEnd, 11U},
        InputActionBinding{GestureType::Cancelled, 12U},
    };
    auto created = InputActionMap::create(InputActionMapConfig{}, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    const std::array events{
        gesture(GestureType::DragEnd, 1U, 3U, true),
        gesture(GestureType::Cancelled, 2U, 3U, true),
    };
    auto actions = map.update(events);
    EXPECT(actions);
    EXPECT(actions.value().size() == 2U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Cancelled);
    EXPECT(actions.value()[1].phase == InputActionPhase::Cancelled);
    return std::nullopt;
}

std::optional<std::string> rebinding_is_validated_and_atomic() {
    const std::array initial{
        InputActionBinding{GestureType::Tap, 10U},
    };
    auto created = InputActionMap::create(InputActionMapConfig{}, initial);
    EXPECT(created);
    auto map = std::move(created).value();

    const std::array invalid_replacement{
        InputActionBinding{GestureType::Swipe, 20U},
        InputActionBinding{GestureType::Swipe, 20U},
    };
    auto rejected = map.replace_bindings(invalid_replacement);
    EXPECT(!rejected);
    EXPECT(map.bindings().size() == 1U);
    EXPECT(map.bindings()[0].gesture == GestureType::Tap);
    EXPECT(map.bindings()[0].action_id == 10U);

    const std::array replacement{
        InputActionBinding{GestureType::Swipe, 20U},
        InputActionBinding{GestureType::LongPress, 21U},
    };
    EXPECT(map.replace_bindings(replacement));
    EXPECT(map.bindings().size() == 2U);
    EXPECT(map.bindings()[0].gesture == GestureType::Swipe);
    return std::nullopt;
}

std::optional<std::string> output_capacity_exhaustion_is_atomic() {
    const std::array bindings{
        InputActionBinding{GestureType::Tap, 1U},
        InputActionBinding{GestureType::Tap, 2U},
    };
    InputActionMapConfig config{};
    config.maximum_action_events_per_update = 1U;
    auto created = InputActionMap::create(config, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    const GestureEvent tap = gesture(GestureType::Tap, 1U);
    auto result = map.update(std::span<const GestureEvent>{&tap, 1U});
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::ResourceExhausted);

    auto empty = map.update({});
    EXPECT(empty);
    EXPECT(empty.value().empty());
    return std::nullopt;
}

std::optional<std::string> invalid_values_and_timestamps_are_rejected() {
    const std::array bindings{
        InputActionBinding{GestureType::Tap, 1U},
    };
    auto created = InputActionMap::create(InputActionMapConfig{}, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    GestureEvent invalid = gesture(GestureType::Tap, 1U);
    invalid.x = std::numeric_limits<double>::quiet_NaN();
    auto result = map.update(std::span<const GestureEvent>{&invalid, 1U});
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);

    GestureEvent inconsistent_cancel = gesture(GestureType::Tap, 1U);
    inconsistent_cancel.cancelled = true;
    result = map.update(
        std::span<const GestureEvent>{&inconsistent_cancel, 1U});
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);

    const GestureEvent first = gesture(GestureType::Tap, 100U);
    EXPECT(map.update(std::span<const GestureEvent>{&first, 1U}));
    const GestureEvent stale = gesture(GestureType::Tap, 99U);
    result = map.update(std::span<const GestureEvent>{&stale, 1U});
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);

    const std::array unordered{
        gesture(GestureType::Tap, 101U),
        gesture(GestureType::Tap, 100U),
    };
    result = map.update(unordered);
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

std::optional<std::string> gesture_batch_limit_is_enforced() {
    InputActionMapConfig config{};
    config.maximum_gesture_events_per_update = 1U;
    const std::array bindings{
        InputActionBinding{GestureType::Tap, 1U},
    };
    auto created = InputActionMap::create(config, bindings);
    EXPECT(created);
    auto map = std::move(created).value();

    const std::array two_events{
        gesture(GestureType::Tap, 1U),
        gesture(GestureType::Tap, 2U),
    };
    const auto result = map.update(two_events);
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::ResourceExhausted);
    return std::nullopt;
}


std::optional<std::string> touch_to_gesture_to_action_pipeline() {
    auto touch_created = Storm::Input::TouchInput::create();
    EXPECT(touch_created);
    auto touch = std::move(touch_created).value();
    EXPECT(touch.attach_surface(8U));

    auto recognizer_created = Storm::Input::GestureRecognizer::create();
    EXPECT(recognizer_created);
    auto recognizer = std::move(recognizer_created).value();

    const std::array bindings{
        InputActionBinding{GestureType::Tap, 500U},
    };
    auto map_created = InputActionMap::create(InputActionMapConfig{}, bindings);
    EXPECT(map_created);
    auto map = std::move(map_created).value();

    const PointerEvent down{
        .pointer_id = 4U,
        .phase = Storm::Input::PointerPhase::Down,
        .x = 12.0,
        .y = 16.0,
        .timestamp_ns = 100U,
        .surface_generation = 8U,
    };
    EXPECT(touch.process(down));

    auto recognized = recognizer.update(
        touch.frame_events(), 100U, touch.surface_generation());
    EXPECT(recognized);
    EXPECT(recognized.value().empty());

    touch.begin_frame();
    const PointerEvent up{
        .pointer_id = 4U,
        .phase = Storm::Input::PointerPhase::Up,
        .x = 12.0,
        .y = 16.0,
        .timestamp_ns = 110U,
        .surface_generation = 8U,
    };
    EXPECT(touch.process(up));

    recognized = recognizer.update(
        touch.frame_events(), 110U, touch.surface_generation());
    EXPECT(recognized);
    EXPECT(recognized.value().size() == 1U);
    EXPECT(recognized.value()[0].type == GestureType::Tap);

    const auto actions = map.update(recognized.value());
    EXPECT(actions);
    EXPECT(actions.value().size() == 1U);
    EXPECT(actions.value()[0].action_id == 500U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Performed);
    EXPECT(actions.value()[0].source.pointer_id == 4U);
    EXPECT(actions.value()[0].source.timestamp_ns == 110U);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"configuration_and_binding_validation",
                 &configuration_and_binding_validation},
        TestCase{"maps_gesture_lifecycle_to_action_phases",
                 &maps_gesture_lifecycle_to_action_phases},
        TestCase{"one_gesture_can_trigger_multiple_actions_in_binding_order",
                 &one_gesture_can_trigger_multiple_actions_in_binding_order},
        TestCase{"cancelled_gestures_produce_cancelled_actions",
                 &cancelled_gestures_produce_cancelled_actions},
        TestCase{"rebinding_is_validated_and_atomic",
                 &rebinding_is_validated_and_atomic},
        TestCase{"output_capacity_exhaustion_is_atomic",
                 &output_capacity_exhaustion_is_atomic},
        TestCase{"invalid_values_and_timestamps_are_rejected",
                 &invalid_values_and_timestamps_are_rejected},
        TestCase{"gesture_batch_limit_is_enforced",
                 &gesture_batch_limit_is_enforced},
        TestCase{"touch_to_gesture_to_action_pipeline",
                 &touch_to_gesture_to_action_pipeline},
    };

    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto failure = test.run();
        if (failure.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *failure << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }

    std::cout << "Executed " << tests.size()
              << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
