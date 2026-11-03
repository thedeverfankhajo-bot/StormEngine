#include "storm/runtime/gesture_action_input_pipeline.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {

using Storm::Input::GestureType;
using Storm::Input::InputActionBinding;
using Storm::Input::InputActionPhase;
using Storm::Input::PointerEvent;
using Storm::Input::PointerPhase;
using Storm::Input::TouchInput;
using Storm::Runtime::GestureActionInputPipeline;
using Storm::Runtime::GestureActionInputPipelineConfig;
using Storm::Runtime::TouchInputFrameView;

#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

TouchInputFrameView frame_view(const TouchInput& input) {
    return TouchInputFrameView{
        .events = input.frame_events(),
        .active_pointers = input.active_pointers(),
        .surface_generation = input.surface_generation(),
    };
}

std::optional<std::string> tap_flows_from_touch_to_game_action() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(7U));

    const std::array bindings{InputActionBinding{GestureType::Tap, 101U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 4U, .phase = PointerPhase::Down,
        .x = 12.0, .y = 16.0, .timestamp_ns = 100U,
        .surface_generation = 7U,
    }));
    auto actions = pipeline.process_frame(frame_view(input), 100U);
    EXPECT(actions);
    EXPECT(actions.value().empty());

    input.begin_frame();
    EXPECT(input.process(PointerEvent{
        .pointer_id = 4U, .phase = PointerPhase::Up,
        .x = 12.0, .y = 16.0, .timestamp_ns = 150U,
        .surface_generation = 7U,
    }));
    actions = pipeline.process_frame(frame_view(input), 150U);
    EXPECT(actions);
    EXPECT(actions.value().size() == 1U);
    EXPECT(actions.value()[0].action_id == 101U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Performed);
    EXPECT(actions.value()[0].source.type == GestureType::Tap);
    EXPECT(actions.value()[0].source.pointer_id == 4U);
    EXPECT(actions.value()[0].source.timestamp_ns == 150U);
    return std::nullopt;
}

std::optional<std::string> long_press_advances_on_frames_without_touch_events() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(9U));

    const std::array bindings{InputActionBinding{GestureType::LongPress, 202U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 8U, .phase = PointerPhase::Down,
        .x = 3.0, .y = 5.0, .timestamp_ns = 100U,
        .surface_generation = 9U,
    }));
    auto actions = pipeline.process_frame(frame_view(input), 100U);
    EXPECT(actions);
    EXPECT(actions.value().empty());

    input.begin_frame();
    EXPECT(input.frame_events().empty());
    actions = pipeline.process_frame(frame_view(input), 500'000'100U);
    EXPECT(actions);
    EXPECT(actions.value().size() == 1U);
    EXPECT(actions.value()[0].action_id == 202U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Performed);
    EXPECT(actions.value()[0].source.type == GestureType::LongPress);
    EXPECT(actions.value()[0].source.timestamp_ns == 500'000'100U);
    return std::nullopt;
}

std::optional<std::string> touch_cancellation_becomes_a_cancelled_action() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(11U));

    const std::array bindings{InputActionBinding{GestureType::Cancelled, 303U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 12U, .phase = PointerPhase::Down,
        .x = 20.0, .y = 30.0, .timestamp_ns = 10U,
        .surface_generation = 11U,
    }));
    auto actions = pipeline.process_frame(frame_view(input), 10U);
    EXPECT(actions);
    EXPECT(actions.value().empty());

    input.begin_frame();
    EXPECT(input.cancel_all(20U));
    actions = pipeline.process_frame(frame_view(input), 20U);
    EXPECT(actions);
    EXPECT(actions.value().size() == 1U);
    EXPECT(actions.value()[0].action_id == 303U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Cancelled);
    EXPECT(actions.value()[0].source.cancelled);
    return std::nullopt;
}

std::optional<std::string> surface_generation_change_cancels_tracked_pointers() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(1U));

    const std::array bindings{InputActionBinding{GestureType::Cancelled, 404U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 21U, .phase = PointerPhase::Down,
        .x = 1.0, .y = 2.0, .timestamp_ns = 100U,
        .surface_generation = 1U,
    }));
    EXPECT(pipeline.process_frame(frame_view(input), 100U));

    input.begin_frame();
    TouchInputFrameView recreated_surface = frame_view(input);
    recreated_surface.surface_generation = 2U;
    const auto actions = pipeline.process_frame(recreated_surface, 101U);
    EXPECT(actions);
    EXPECT(actions.value().size() == 1U);
    EXPECT(actions.value()[0].action_id == 404U);
    EXPECT(actions.value()[0].phase == InputActionPhase::Cancelled);
    EXPECT(actions.value()[0].source.pointer_id == 21U);
    return std::nullopt;
}

std::optional<std::string> non_monotonic_frame_time_is_rejected() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(2U));

    auto pipeline_result = GestureActionInputPipeline::create();
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(pipeline.process_frame(frame_view(input), 200U));
    const auto stale = pipeline.process_frame(frame_view(input), 199U);
    EXPECT(!stale);
    EXPECT(stale.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

std::optional<std::string> action_output_exhaustion_has_no_partial_result() {
    GestureActionInputPipelineConfig config{};
    config.actions.maximum_action_events_per_update = 1U;
    const std::array bindings{
        InputActionBinding{GestureType::Tap, 1U},
        InputActionBinding{GestureType::Tap, 2U},
    };
    auto pipeline_result = GestureActionInputPipeline::create(config, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(3U));

    EXPECT(input.process(PointerEvent{
        .pointer_id = 5U, .phase = PointerPhase::Down,
        .x = 7.0, .y = 9.0, .timestamp_ns = 300U,
        .surface_generation = 3U,
    }));
    EXPECT(pipeline.process_frame(frame_view(input), 300U));

    input.begin_frame();
    EXPECT(input.process(PointerEvent{
        .pointer_id = 5U, .phase = PointerPhase::Up,
        .x = 7.0, .y = 9.0, .timestamp_ns = 350U,
        .surface_generation = 3U,
    }));
    const auto exhausted = pipeline.process_frame(frame_view(input), 350U);
    EXPECT(!exhausted);
    EXPECT(exhausted.error().code == Storm::ErrorCode::ResourceExhausted);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"tap_flows_from_touch_to_game_action",
                 &tap_flows_from_touch_to_game_action},
        TestCase{"long_press_advances_on_frames_without_touch_events",
                 &long_press_advances_on_frames_without_touch_events},
        TestCase{"touch_cancellation_becomes_a_cancelled_action",
                 &touch_cancellation_becomes_a_cancelled_action},
        TestCase{"surface_generation_change_cancels_tracked_pointers",
                 &surface_generation_change_cancels_tracked_pointers},
        TestCase{"non_monotonic_frame_time_is_rejected",
                 &non_monotonic_frame_time_is_rejected},
        TestCase{"action_output_exhaustion_has_no_partial_result",
                 &action_output_exhaustion_has_no_partial_result},
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
