#include "storm/core/game_loop.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

enum class Call {
    Input,
    Events,
    FixedUpdate,
    VariableUpdate,
    Render,
};

class RecordingCallbacks final : public Storm::IGameLoopCallbacks {
public:
    [[nodiscard]] Storm::Result<void> process_input() override {
        calls.push_back(Call::Input);
        return result_for(Call::Input);
    }

    [[nodiscard]] Storm::Result<void> process_events() override {
        calls.push_back(Call::Events);
        return result_for(Call::Events);
    }

    [[nodiscard]] Storm::Result<void> fixed_update(
        std::chrono::nanoseconds fixed_delta) override {
        calls.push_back(Call::FixedUpdate);
        fixed_deltas.push_back(fixed_delta);
        ++fixed_update_count;
        return result_for(Call::FixedUpdate);
    }

    [[nodiscard]] Storm::Result<void> variable_update(
        std::chrono::nanoseconds accepted_frame_delta) override {
        calls.push_back(Call::VariableUpdate);
        variable_delta = accepted_frame_delta;
        return result_for(Call::VariableUpdate);
    }

    [[nodiscard]] Storm::Result<void> render(double interpolation_alpha) override {
        calls.push_back(Call::Render);
        render_alpha = interpolation_alpha;
        return result_for(Call::Render);
    }

    Call fail_on{Call::Render};
    bool should_fail{false};
    std::size_t fail_fixed_update_number{1U};
    std::size_t fixed_update_count{0U};
    std::vector<Call> calls;
    std::vector<std::chrono::nanoseconds> fixed_deltas;
    std::chrono::nanoseconds variable_delta{0};
    double render_alpha{0.0};

private:
    [[nodiscard]] Storm::Result<void> result_for(Call call) const {
        const bool matching_call = should_fail && fail_on == call;
        const bool matching_fixed_number =
            call != Call::FixedUpdate ||
            fixed_update_count == fail_fixed_update_number;
        if (matching_call && matching_fixed_number) {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure,
                "injected callback failure"));
        }
        return Storm::Result<void>::success();
    }
};

[[nodiscard]] std::optional<std::string> test_config_validation() {
    auto invalid = Storm::GameLoop::create(
        Storm::FixedStepConfig{.fixed_step = 0ns});
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_frame_phases_run_in_order() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;

    auto result = loop.tick(25ms, callbacks);
    EXPECT(result);
    constexpr std::array expected{
        Call::Input,
        Call::Events,
        Call::FixedUpdate,
        Call::FixedUpdate,
        Call::VariableUpdate,
        Call::Render,
    };
    EXPECT(callbacks.calls.size() == expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        EXPECT(callbacks.calls[index] == expected[index]);
    }
    EXPECT(result.value().step_count == 2U);
    EXPECT(result.value().interpolation_alpha == 0.5);
    EXPECT(callbacks.fixed_deltas.size() == 2U);
    EXPECT(callbacks.fixed_deltas[0] == 10ms);
    EXPECT(callbacks.fixed_deltas[1] == 10ms);
    EXPECT(callbacks.variable_delta == 25ms);
    EXPECT(callbacks.render_alpha == 0.5);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_zero_delta_runs_frame_phases_without_fixed_update() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;

    auto result = loop.tick(0ns, callbacks);
    EXPECT(result);
    constexpr std::array expected{
        Call::Input,
        Call::Events,
        Call::VariableUpdate,
        Call::Render,
    };
    EXPECT(callbacks.calls.size() == expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        EXPECT(callbacks.calls[index] == expected[index]);
    }
    EXPECT(result.value().step_count == 0U);
    EXPECT(result.value().interpolation_alpha == 0.0);
    EXPECT(callbacks.variable_delta == 0ns);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_clamped_time_and_discarded_backlog_are_reported() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 50ms,
            .maximum_steps_per_frame = 3U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;

    auto result = loop.tick(100ms, callbacks);
    EXPECT(result);
    EXPECT(result.value().accepted_frame_delta == 50ms);
    EXPECT(result.value().clamped_frame_delta == 50ms);
    EXPECT(result.value().step_count == 3U);
    EXPECT(result.value().discarded_backlog == 20ms);
    EXPECT(callbacks.fixed_update_count == 3U);
    EXPECT(callbacks.variable_delta == 50ms);
    EXPECT(callbacks.render_alpha == 0.0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_fixed_update_failure_aborts_remaining_phases() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;
    callbacks.should_fail = true;
    callbacks.fail_on = Call::FixedUpdate;
    callbacks.fail_fixed_update_number = 2U;

    auto result = loop.tick(35ms, callbacks);
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::IoFailure);
    EXPECT(result.error().message.find("GameLoop::fixed_update failed") == 0U);
    constexpr std::array expected{
        Call::Input,
        Call::Events,
        Call::FixedUpdate,
        Call::FixedUpdate,
    };
    EXPECT(callbacks.calls.size() == expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        EXPECT(callbacks.calls[index] == expected[index]);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_variable_update_failure_skips_render() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;
    callbacks.should_fail = true;
    callbacks.fail_on = Call::VariableUpdate;

    auto result = loop.tick(15ms, callbacks);
    EXPECT(!result);
    EXPECT(result.error().message.find("GameLoop::variable_update failed") == 0U);
    constexpr std::array expected{
        Call::Input,
        Call::Events,
        Call::FixedUpdate,
        Call::VariableUpdate,
    };
    EXPECT(callbacks.calls.size() == expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        EXPECT(callbacks.calls[index] == expected[index]);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_input_failure_stops_before_event_and_timing_work() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;
    callbacks.should_fail = true;
    callbacks.fail_on = Call::Input;

    auto result = loop.tick(10ms, callbacks);
    EXPECT(!result);
    EXPECT(result.error().message.find("GameLoop::process_input failed") == 0U);
    EXPECT(callbacks.calls.size() == 1U);
    EXPECT(callbacks.calls.front() == Call::Input);

    // Input failed before the scheduler was advanced, so the next valid frame
    // still receives the time supplied to that next call only.
    callbacks.should_fail = false;
    callbacks.calls.clear();
    auto next = loop.tick(10ms, callbacks);
    EXPECT(next);
    EXPECT(next.value().step_count == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_negative_delta_is_rejected_before_callbacks() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;

    auto result = loop.tick(-1ms, callbacks);
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(callbacks.calls.empty());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_reset_timing_discards_suspended_remainder() {
    auto created = Storm::GameLoop::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto loop = std::move(created).value();
    RecordingCallbacks callbacks;

    auto before_pause = loop.tick(5ms, callbacks);
    EXPECT(before_pause);
    EXPECT(before_pause.value().step_count == 0U);

    loop.reset_timing();
    callbacks.calls.clear();
    callbacks.fixed_deltas.clear();
    callbacks.fixed_update_count = 0U;

    auto after_resume = loop.tick(5ms, callbacks);
    EXPECT(after_resume);
    EXPECT(after_resume.value().step_count == 0U);
    EXPECT(after_resume.value().interpolation_alpha == 0.5);
    EXPECT(callbacks.fixed_update_count == 0U);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"config_validation", &test_config_validation},
        TestCase{"frame_phases_run_in_order", &test_frame_phases_run_in_order},
        TestCase{"zero_delta_runs_frame_phases_without_fixed_update", &test_zero_delta_runs_frame_phases_without_fixed_update},
        TestCase{"clamped_time_and_discarded_backlog_are_reported", &test_clamped_time_and_discarded_backlog_are_reported},
        TestCase{"fixed_update_failure_aborts_remaining_phases", &test_fixed_update_failure_aborts_remaining_phases},
        TestCase{"variable_update_failure_skips_render", &test_variable_update_failure_skips_render},
        TestCase{"input_failure_stops_before_event_and_timing_work", &test_input_failure_stops_before_event_and_timing_work},
        TestCase{"negative_delta_is_rejected_before_callbacks", &test_negative_delta_is_rejected_before_callbacks},
        TestCase{"reset_timing_discards_suspended_remainder", &test_reset_timing_discards_suspended_remainder},
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

    std::cout << "Executed " << std::size(tests)
              << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
