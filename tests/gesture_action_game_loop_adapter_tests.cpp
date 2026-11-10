#include "storm/runtime/gesture_action_game_loop_adapter.hpp"
#include "storm/runtime/mobile_frame_driver.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

using Storm::Input::GestureType;
using Storm::Input::InputActionBinding;
using Storm::Input::InputActionEvent;
using Storm::Input::InputActionPhase;
using Storm::Input::PointerEvent;
using Storm::Input::PointerPhase;
using Storm::Input::TouchInput;
using Storm::Runtime::GestureActionGameLoopAdapter;
using Storm::Runtime::GestureActionInputPipeline;
using Storm::Runtime::GestureActionInputPipelineConfig;
using Storm::Runtime::IGameActionFrameConsumer;
using Storm::Runtime::MobileFrameDriver;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

struct Trace final {
    std::vector<std::string> calls;
};

class RecordingCallbacks final : public Storm::IGameLoopCallbacks {
public:
    explicit RecordingCallbacks(Trace& trace) : trace_(trace) {}

    [[nodiscard]] Storm::Result<void> begin_frame(
        std::chrono::nanoseconds timestamp) override {
        trace_.calls.emplace_back("begin_frame");
        last_timestamp = timestamp;
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> process_input() override {
        trace_.calls.emplace_back("input");
        ++input_calls;
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> process_events() override {
        trace_.calls.emplace_back("events");
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> fixed_update(
        std::chrono::nanoseconds) override {
        trace_.calls.emplace_back("fixed");
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> variable_update(
        std::chrono::nanoseconds) override {
        trace_.calls.emplace_back("variable");
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> render(double) override {
        trace_.calls.emplace_back("render");
        return Storm::Result<void>::success();
    }

    Trace& trace_;
    std::chrono::nanoseconds last_timestamp{0};
    std::size_t input_calls{0U};
};

class RecordingActionConsumer final : public IGameActionFrameConsumer {
public:
    explicit RecordingActionConsumer(Trace& trace) : trace_(trace) {}

    [[nodiscard]] Storm::Result<void> process_game_actions(
        std::span<const InputActionEvent> actions) override {
        trace_.calls.emplace_back("actions");
        ++calls;
        received.assign(actions.begin(), actions.end());
        if (fail) {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure,
                "injected game-action consumer failure"));
        }
        return Storm::Result<void>::success();
    }

    Trace& trace_;
    std::vector<InputActionEvent> received;
    std::size_t calls{0U};
    bool fail{false};
};

[[nodiscard]] bool activate(
    Storm::Platform::LifecycleStateMachine& lifecycle) {
    const auto inactive = lifecycle.transition_to(
        Storm::Platform::LifecycleState::ForegroundInactive);
    if (!inactive) {
        return false;
    }
    return static_cast<bool>(lifecycle.transition_to(
        Storm::Platform::LifecycleState::ForegroundActive));
}

[[nodiscard]] Storm::FixedStepConfig frame_config() {
    return Storm::FixedStepConfig{
        .fixed_step = 1ms,
        .maximum_frame_delta = 100ms,
        .maximum_steps_per_frame = 8U,
    };
}

[[nodiscard]] std::optional<std::string>
touch_gestures_reach_gameplay_before_application_input() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();

    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(activate(lifecycle));
    const auto attached = surface.attach(7U, {320U, 640U}, 1.0);
    EXPECT(attached);
    EXPECT(input.attach_surface(attached.value().generation));

    const std::array bindings{InputActionBinding{GestureType::Tap, 41U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 3U,
        .phase = PointerPhase::Down,
        .x = 12.0,
        .y = 16.0,
        .timestamp_ns = 100U,
        .surface_generation = attached.value().generation,
    }));

    auto driver_result = MobileFrameDriver::create(
        frame_config(), lifecycle, surface);
    EXPECT(driver_result);
    auto driver = std::move(driver_result).value();

    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingActionConsumer consumer(trace);
    GestureActionGameLoopAdapter adapter(callbacks, input, pipeline, consumer);

    EXPECT(driver.tick_at_monotonic_nanoseconds(100, adapter));
    EXPECT(consumer.calls == 0U);
    EXPECT(input.frame_events().empty());

    trace.calls.clear();
    EXPECT(input.process(PointerEvent{
        .pointer_id = 3U,
        .phase = PointerPhase::Up,
        .x = 12.0,
        .y = 16.0,
        .timestamp_ns = 150U,
        .surface_generation = attached.value().generation,
    }));
    const auto frame = driver.tick_at_monotonic_nanoseconds(150, adapter);
    EXPECT(frame);
    EXPECT(frame.value().disposition ==
        Storm::Runtime::FrameDisposition::Executed);
    EXPECT(consumer.calls == 1U);
    EXPECT(consumer.received.size() == 1U);
    EXPECT(consumer.received[0].action_id == 41U);
    EXPECT(consumer.received[0].phase == InputActionPhase::Performed);
    EXPECT(consumer.received[0].source.type == GestureType::Tap);
    EXPECT(callbacks.last_timestamp == std::chrono::nanoseconds{150});
    EXPECT(input.frame_events().empty());

    constexpr std::array expected{
        "begin_frame", "actions", "input", "events", "variable", "render"};
    EXPECT(trace.calls.size() == expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        EXPECT(trace.calls[index] == expected[index]);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string>
long_press_uses_frame_time_when_no_pointer_event_arrives() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();

    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(activate(lifecycle));
    const auto attached = surface.attach(8U, {320U, 640U}, 1.0);
    EXPECT(attached);
    EXPECT(input.attach_surface(attached.value().generation));

    GestureActionInputPipelineConfig config{};
    config.gesture.long_press_duration_ns = 50U;
    const std::array bindings{InputActionBinding{GestureType::LongPress, 52U}};
    auto pipeline_result = GestureActionInputPipeline::create(config, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 5U,
        .phase = PointerPhase::Down,
        .x = 4.0,
        .y = 6.0,
        .timestamp_ns = 1'000U,
        .surface_generation = attached.value().generation,
    }));

    auto driver_result = MobileFrameDriver::create(
        frame_config(), lifecycle, surface);
    EXPECT(driver_result);
    auto driver = std::move(driver_result).value();

    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingActionConsumer consumer(trace);
    GestureActionGameLoopAdapter adapter(callbacks, input, pipeline, consumer);

    EXPECT(driver.tick_at_monotonic_nanoseconds(1'000, adapter));
    EXPECT(input.frame_events().empty());
    EXPECT(consumer.calls == 0U);

    const auto frame = driver.tick_at_monotonic_nanoseconds(1'050, adapter);
    EXPECT(frame);
    EXPECT(consumer.calls == 1U);
    EXPECT(consumer.received.size() == 1U);
    EXPECT(consumer.received[0].action_id == 52U);
    EXPECT(consumer.received[0].source.type == GestureType::LongPress);
    EXPECT(consumer.received[0].source.timestamp_ns == 1'050U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string>
consumer_failure_clears_input_and_does_not_replay_actions() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();

    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(activate(lifecycle));
    const auto attached = surface.attach(9U, {320U, 640U}, 1.0);
    EXPECT(attached);
    EXPECT(input.attach_surface(attached.value().generation));

    const std::array bindings{InputActionBinding{GestureType::Tap, 61U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 6U,
        .phase = PointerPhase::Down,
        .x = 1.0,
        .y = 2.0,
        .timestamp_ns = 200U,
        .surface_generation = attached.value().generation,
    }));

    auto driver_result = MobileFrameDriver::create(
        frame_config(), lifecycle, surface);
    EXPECT(driver_result);
    auto driver = std::move(driver_result).value();

    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingActionConsumer consumer(trace);
    GestureActionGameLoopAdapter adapter(callbacks, input, pipeline, consumer);

    EXPECT(driver.tick_at_monotonic_nanoseconds(200, adapter));
    EXPECT(input.process(PointerEvent{
        .pointer_id = 6U,
        .phase = PointerPhase::Up,
        .x = 1.0,
        .y = 2.0,
        .timestamp_ns = 250U,
        .surface_generation = attached.value().generation,
    }));

    consumer.fail = true;
    const auto failed = driver.tick_at_monotonic_nanoseconds(250, adapter);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::IoFailure);
    EXPECT(consumer.calls == 1U);
    EXPECT(consumer.received.size() == 1U);
    EXPECT(callbacks.input_calls == 1U);
    EXPECT(input.frame_events().empty());

    consumer.fail = false;
    const auto recovered = driver.tick_at_monotonic_nanoseconds(260, adapter);
    EXPECT(recovered);
    EXPECT(consumer.calls == 1U);
    EXPECT(callbacks.input_calls == 2U);
    EXPECT(input.frame_events().empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string>
negative_frame_time_is_rejected_without_consuming_touch_events() {
    auto input_result = TouchInput::create();
    EXPECT(input_result);
    auto input = std::move(input_result).value();

    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(activate(lifecycle));
    const auto attached = surface.attach(10U, {320U, 640U}, 1.0);
    EXPECT(attached);
    EXPECT(input.attach_surface(attached.value().generation));

    const std::array bindings{InputActionBinding{GestureType::Tap, 71U}};
    auto pipeline_result = GestureActionInputPipeline::create(
        GestureActionInputPipelineConfig{}, bindings);
    EXPECT(pipeline_result);
    auto pipeline = std::move(pipeline_result).value();

    EXPECT(input.process(PointerEvent{
        .pointer_id = 7U,
        .phase = PointerPhase::Down,
        .x = 2.0,
        .y = 3.0,
        .timestamp_ns = 0U,
        .surface_generation = attached.value().generation,
    }));

    auto driver_result = MobileFrameDriver::create(
        frame_config(), lifecycle, surface);
    EXPECT(driver_result);
    auto driver = std::move(driver_result).value();

    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingActionConsumer consumer(trace);
    GestureActionGameLoopAdapter adapter(callbacks, input, pipeline, consumer);

    const auto invalid = driver.tick_at_monotonic_nanoseconds(-1, adapter);
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(input.frame_events().size() == 1U);
    EXPECT(callbacks.input_calls == 0U);

    const auto recovered = driver.tick_at_monotonic_nanoseconds(1, adapter);
    EXPECT(recovered);
    EXPECT(input.frame_events().empty());
    EXPECT(callbacks.input_calls == 1U);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{
            "touch_gestures_reach_gameplay_before_application_input",
            &touch_gestures_reach_gameplay_before_application_input},
        TestCase{
            "long_press_uses_frame_time_when_no_pointer_event_arrives",
            &long_press_uses_frame_time_when_no_pointer_event_arrives},
        TestCase{
            "consumer_failure_clears_input_and_does_not_replay_actions",
            &consumer_failure_clears_input_and_does_not_replay_actions},
        TestCase{
            "negative_frame_time_is_rejected_without_consuming_touch_events",
            &negative_frame_time_is_rejected_without_consuming_touch_events},
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
