#include "storm/core/fixed_step_scheduler.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

using namespace std::chrono_literals;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_config_validation() {
    auto zero_step = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{.fixed_step = 0ns});
    EXPECT(!zero_step);
    EXPECT(zero_step.error().code == Storm::ErrorCode::InvalidArgument);

    auto negative_frame_limit = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{.maximum_frame_delta = -1ns});
    EXPECT(!negative_frame_limit);
    EXPECT(negative_frame_limit.error().code == Storm::ErrorCode::InvalidArgument);

    auto zero_step_budget = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{.maximum_steps_per_frame = 0U});
    EXPECT(!zero_step_budget);
    EXPECT(zero_step_budget.error().code == Storm::ErrorCode::InvalidArgument);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_partial_deltas_and_fixed_steps() {
    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 8U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    auto partial = scheduler.advance(4ms);
    EXPECT(partial);
    EXPECT(partial.value().step_count == 0U);
    EXPECT(std::abs(partial.value().interpolation_alpha - 0.4) < 1e-12);
    EXPECT(scheduler.remaining_accumulator() == 4ms);

    auto completed_step = scheduler.advance(6ms);
    EXPECT(completed_step);
    EXPECT(completed_step.value().step_count == 1U);
    EXPECT(completed_step.value().interpolation_alpha == 0.0);
    EXPECT(scheduler.remaining_accumulator() == 0ns);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_frame_delta_clamping() {
    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 50ms,
            .maximum_steps_per_frame = 3U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    auto advanced = scheduler.advance(100ms);
    EXPECT(advanced);
    EXPECT(advanced.value().accepted_frame_delta == 50ms);
    EXPECT(advanced.value().clamped_frame_delta == 50ms);
    EXPECT(advanced.value().step_count == 3U);
    EXPECT(advanced.value().discarded_backlog == 20ms);
    EXPECT(scheduler.remaining_accumulator() == 0ns);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_catch_up_budget_preserves_fraction() {
    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 3U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    auto advanced = scheduler.advance(45ms);
    EXPECT(advanced);
    EXPECT(advanced.value().step_count == 3U);
    EXPECT(advanced.value().discarded_backlog == 10ms);
    EXPECT(scheduler.remaining_accumulator() == 5ms);
    EXPECT(std::abs(advanced.value().interpolation_alpha - 0.5) < 1e-12);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_negative_delta_does_not_mutate_state() {
    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 4U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    EXPECT(scheduler.advance(5ms));
    auto invalid = scheduler.advance(-1ms);
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(scheduler.remaining_accumulator() == 5ms);

    auto completed_step = scheduler.advance(5ms);
    EXPECT(completed_step);
    EXPECT(completed_step.value().step_count == 1U);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_reset_clears_accumulator() {
    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = 10ms,
            .maximum_frame_delta = 100ms,
            .maximum_steps_per_frame = 4U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    EXPECT(scheduler.advance(7ms));
    EXPECT(scheduler.remaining_accumulator() == 7ms);
    scheduler.reset();
    EXPECT(scheduler.remaining_accumulator() == 0ns);

    auto after_reset = scheduler.advance(5ms);
    EXPECT(after_reset);
    EXPECT(after_reset.value().step_count == 0U);
    EXPECT(std::abs(after_reset.value().interpolation_alpha - 0.5) < 1e-12);

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_accumulator_overflow_is_reported_without_mutation() {
    using Duration = std::chrono::nanoseconds;
    const auto maximum = Duration::max();

    auto created = Storm::FixedStepScheduler::create(
        Storm::FixedStepConfig{
            .fixed_step = maximum - 1ns,
            .maximum_frame_delta = maximum,
            .maximum_steps_per_frame = 1U,
        });
    EXPECT(created);
    auto scheduler = std::move(created).value();

    auto first = scheduler.advance(maximum);
    EXPECT(first);
    EXPECT(first.value().step_count == 1U);
    EXPECT(scheduler.remaining_accumulator() == 1ns);

    auto overflow = scheduler.advance(maximum);
    EXPECT(!overflow);
    EXPECT(overflow.error().code == Storm::ErrorCode::ResourceExhausted);
    EXPECT(scheduler.remaining_accumulator() == 1ns);

    auto valid_after_error = scheduler.advance(maximum - 1ns);
    EXPECT(valid_after_error);
    EXPECT(valid_after_error.value().step_count == 1U);
    EXPECT(scheduler.remaining_accumulator() == 1ns);

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
        TestCase{"partial_deltas_and_fixed_steps", &test_partial_deltas_and_fixed_steps},
        TestCase{"frame_delta_clamping", &test_frame_delta_clamping},
        TestCase{"catch_up_budget_preserves_fraction", &test_catch_up_budget_preserves_fraction},
        TestCase{"negative_delta_does_not_mutate_state", &test_negative_delta_does_not_mutate_state},
        TestCase{"reset_clears_accumulator", &test_reset_clears_accumulator},
        TestCase{"accumulator_overflow_is_reported_without_mutation", &test_accumulator_overflow_is_reported_without_mutation},
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
