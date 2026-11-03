#include "storm/runtime/touch_input_game_loop_adapter.hpp"

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

#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

struct Trace final {
    std::vector<std::string> calls;
};

class RecordingCallbacks final : public Storm::IGameLoopCallbacks {
public:
    explicit RecordingCallbacks(Trace& trace) : trace_(trace) {}

    [[nodiscard]] Storm::Result<void> process_input() override {
        trace_.calls.emplace_back("input");
        if (fail_input) {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure, "injected application input failure"));
        }
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

    bool fail_input{false};

private:
    Trace& trace_;
};

class RecordingTouchConsumer final : public Storm::Runtime::ITouchInputFrameConsumer {
public:
    explicit RecordingTouchConsumer(Trace& trace) : trace_(trace) {}

    [[nodiscard]] Storm::Result<void> process_touch_input(
        const Storm::Runtime::TouchInputFrameView& frame) override {
        trace_.calls.emplace_back("touch");
        events.assign(frame.events.begin(), frame.events.end());
        active_pointers.assign(
            frame.active_pointers.begin(), frame.active_pointers.end());
        surface_generation = frame.surface_generation;
        if (fail) {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure, "injected touch consumer failure"));
        }
        return Storm::Result<void>::success();
    }

    std::vector<Storm::Input::PointerEvent> events;
    std::vector<Storm::Input::PointerSnapshot> active_pointers;
    std::optional<std::uint64_t> surface_generation;
    bool fail{false};

private:
    Trace& trace_;
};

[[nodiscard]] Storm::Result<Storm::Input::TouchInput> make_touch_input() {
    return Storm::Input::TouchInput::create();
}

[[nodiscard]] Storm::Result<Storm::GameLoop> make_game_loop() {
    return Storm::GameLoop::create(Storm::FixedStepConfig{
        .fixed_step = 10ms,
        .maximum_frame_delta = 100ms,
        .maximum_steps_per_frame = 4U,
    });
}

[[nodiscard]] std::optional<std::string> frame_input_is_delivered_before_other_phases() {
    auto input_result = make_touch_input();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(7U));
    EXPECT(input.process(Storm::Input::PointerEvent{
        .pointer_id = 11U,
        .phase = Storm::Input::PointerPhase::Down,
        .x = 2.0,
        .y = 3.0,
        .timestamp_ns = 10U,
        .surface_generation = 7U,
    }));
    EXPECT(input.process(Storm::Input::PointerEvent{
        .pointer_id = 11U,
        .phase = Storm::Input::PointerPhase::Move,
        .x = 5.0,
        .y = 8.0,
        .timestamp_ns = 11U,
        .surface_generation = 7U,
    }));

    auto loop_result = make_game_loop();
    EXPECT(loop_result);
    auto loop = std::move(loop_result).value();

    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingTouchConsumer consumer(trace);
    Storm::Runtime::TouchInputGameLoopAdapter adapter(callbacks, input, consumer);

    const auto frame = loop.tick(25ms, adapter);
    EXPECT(frame);
    constexpr std::array expected{
        "touch", "input", "events", "fixed", "fixed", "variable", "render"};
    EXPECT(trace.calls.size() == expected.size());
    for (std::size_t i = 0U; i < expected.size(); ++i) {
        EXPECT(trace.calls[i] == expected[i]);
    }

    EXPECT(consumer.surface_generation == std::optional<std::uint64_t>{7U});
    EXPECT(consumer.events.size() == 2U);
    EXPECT(consumer.events[0].phase == Storm::Input::PointerPhase::Down);
    EXPECT(consumer.events[1].phase == Storm::Input::PointerPhase::Move);
    EXPECT(consumer.active_pointers.size() == 1U);
    EXPECT(consumer.active_pointers[0].x == 5.0);
    EXPECT(consumer.active_pointers[0].y == 8.0);
    EXPECT(consumer.active_pointers[0].delta_x == 3.0);
    EXPECT(consumer.active_pointers[0].delta_y == 5.0);

    EXPECT(input.frame_events().empty());
    EXPECT(input.active_pointer_count() == 1U);
    EXPECT(input.active_pointers()[0].delta_x == 0.0);
    EXPECT(input.active_pointers()[0].delta_y == 0.0);
    EXPECT(frame.value().step_count == 2U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> cancellation_is_delivered_before_state_is_cleared() {
    auto input_result = make_touch_input();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(3U));
    EXPECT(input.process(Storm::Input::PointerEvent{
        .pointer_id = 4U,
        .phase = Storm::Input::PointerPhase::Down,
        .x = 9.0,
        .y = 12.0,
        .timestamp_ns = 20U,
        .surface_generation = 3U,
    }));

    auto loop_result = make_game_loop();
    EXPECT(loop_result);
    auto loop = std::move(loop_result).value();
    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingTouchConsumer consumer(trace);
    Storm::Runtime::TouchInputGameLoopAdapter adapter(callbacks, input, consumer);

    EXPECT(loop.tick(0ns, adapter));
    EXPECT(input.cancel_all(21U));
    EXPECT(input.active_pointer_count() == 0U);
    EXPECT(loop.tick(0ns, adapter));

    EXPECT(consumer.events.size() == 1U);
    EXPECT(consumer.events[0].phase == Storm::Input::PointerPhase::Cancel);
    EXPECT(consumer.events[0].timestamp_ns == 21U);
    EXPECT(consumer.active_pointers.empty());
    EXPECT(input.frame_events().empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> touch_consumer_failure_clears_events_without_replay() {
    auto input_result = make_touch_input();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(1U));
    EXPECT(input.process(Storm::Input::PointerEvent{
        .pointer_id = 1U,
        .phase = Storm::Input::PointerPhase::Down,
        .x = 1.0,
        .y = 1.0,
        .timestamp_ns = 1U,
        .surface_generation = 1U,
    }));

    auto loop_result = make_game_loop();
    EXPECT(loop_result);
    auto loop = std::move(loop_result).value();
    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingTouchConsumer consumer(trace);
    consumer.fail = true;
    Storm::Runtime::TouchInputGameLoopAdapter adapter(callbacks, input, consumer);

    const auto failed = loop.tick(0ns, adapter);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::IoFailure);
    EXPECT(trace.calls.size() == 1U);
    EXPECT(trace.calls[0] == "touch");
    EXPECT(input.frame_events().empty());
    EXPECT(input.active_pointer_count() == 1U);

    consumer.fail = false;
    trace.calls.clear();
    EXPECT(loop.tick(0ns, adapter));
    EXPECT(consumer.events.empty());
    EXPECT(trace.calls.size() == 5U);
    EXPECT(trace.calls[0] == "touch");
    EXPECT(trace.calls[1] == "input");
    EXPECT(trace.calls[2] == "events");
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> application_input_failure_does_not_replay_touch_events() {
    auto input_result = make_touch_input();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(2U));
    EXPECT(input.process(Storm::Input::PointerEvent{
        .pointer_id = 8U,
        .phase = Storm::Input::PointerPhase::Down,
        .x = 4.0,
        .y = 6.0,
        .timestamp_ns = 3U,
        .surface_generation = 2U,
    }));

    auto loop_result = make_game_loop();
    EXPECT(loop_result);
    auto loop = std::move(loop_result).value();
    Trace trace;
    RecordingCallbacks callbacks(trace);
    callbacks.fail_input = true;
    RecordingTouchConsumer consumer(trace);
    Storm::Runtime::TouchInputGameLoopAdapter adapter(callbacks, input, consumer);

    const auto failed = loop.tick(0ns, adapter);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::IoFailure);
    EXPECT(consumer.events.size() == 1U);
    EXPECT(input.frame_events().empty());

    callbacks.fail_input = false;
    trace.calls.clear();
    EXPECT(loop.tick(0ns, adapter));
    EXPECT(consumer.events.empty());
    EXPECT(trace.calls[0] == "touch");
    EXPECT(trace.calls[1] == "input");
    return std::nullopt;
}


[[nodiscard]] std::optional<std::string> native_batches_accumulate_until_consumed() {
    auto input_result = make_touch_input();
    EXPECT(input_result);
    auto input = std::move(input_result).value();
    EXPECT(input.attach_surface(9U));

    const std::array first_callback{
        Storm::Input::PointerEvent{
            .pointer_id = 2U,
            .phase = Storm::Input::PointerPhase::Down,
            .x = 10.0,
            .y = 20.0,
            .timestamp_ns = 100U,
            .surface_generation = 9U,
        },
        Storm::Input::PointerEvent{
            .pointer_id = 2U,
            .phase = Storm::Input::PointerPhase::Move,
            .x = 13.0,
            .y = 24.0,
            .timestamp_ns = 101U,
            .surface_generation = 9U,
        },
    };
    const std::array second_callback{
        Storm::Input::PointerEvent{
            .pointer_id = 2U,
            .phase = Storm::Input::PointerPhase::Move,
            .x = 17.0,
            .y = 29.0,
            .timestamp_ns = 102U,
            .surface_generation = 9U,
        },
    };

    EXPECT(input.process_batch(first_callback));
    EXPECT(input.process_batch(second_callback));
    EXPECT(input.frame_events().size() == 3U);
    EXPECT(input.active_pointers().size() == 1U);
    EXPECT(input.active_pointers()[0].delta_x == 7.0);
    EXPECT(input.active_pointers()[0].delta_y == 9.0);

    // A native CANCEL/lifecycle cancellation is part of the same pending
    // frame and must not erase the earlier Down/Move transitions.
    EXPECT(input.cancel_all(103U));

    auto loop_result = make_game_loop();
    EXPECT(loop_result);
    auto loop = std::move(loop_result).value();
    Trace trace;
    RecordingCallbacks callbacks(trace);
    RecordingTouchConsumer consumer(trace);
    Storm::Runtime::TouchInputGameLoopAdapter adapter(callbacks, input, consumer);

    EXPECT(loop.tick(0ns, adapter));
    EXPECT(consumer.events.size() == 4U);
    EXPECT(consumer.events[0].phase == Storm::Input::PointerPhase::Down);
    EXPECT(consumer.events[1].phase == Storm::Input::PointerPhase::Move);
    EXPECT(consumer.events[2].phase == Storm::Input::PointerPhase::Move);
    EXPECT(consumer.events[3].phase == Storm::Input::PointerPhase::Cancel);
    EXPECT(consumer.events[3].timestamp_ns == 103U);
    EXPECT(consumer.active_pointers.empty());
    EXPECT(input.frame_events().empty());
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"frame_input_is_delivered_before_other_phases",
                 &frame_input_is_delivered_before_other_phases},
        TestCase{"cancellation_is_delivered_before_state_is_cleared",
                 &cancellation_is_delivered_before_state_is_cleared},
        TestCase{"touch_consumer_failure_clears_events_without_replay",
                 &touch_consumer_failure_clears_events_without_replay},
        TestCase{"application_input_failure_does_not_replay_touch_events",
                 &application_input_failure_does_not_replay_touch_events},
        TestCase{"native_batches_accumulate_until_consumed",
                 &native_batches_accumulate_until_consumed},
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
