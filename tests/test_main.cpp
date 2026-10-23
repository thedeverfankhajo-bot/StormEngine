#include "storm/core/fixed_step_clock.hpp"
#include "storm/core/runtime_lifecycle.hpp"
#include "storm/core/status.hpp"
#include "storm/core/version.hpp"
#include "storm/math/vector.hpp"
#include "storm/scene/dimension.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view expression, int line) {
    if (!condition) {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define STORM_CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

[[nodiscard]] bool nearly_equal(double left, double right, double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

void test_version() {
    STORM_CHECK(storm::version() == storm::kVersion);
    STORM_CHECK(storm::version() == (storm::Version{0, 1, 0}));
    STORM_CHECK(storm::version_string() == "0.1.0");
}

void test_status_values() {
    using storm::core::ErrorCode;
    using storm::core::Status;

    const auto success = Status::success();
    STORM_CHECK(success.ok());
    STORM_CHECK(static_cast<bool>(success));
    STORM_CHECK(success.code() == ErrorCode::None);
    STORM_CHECK(success.message().empty());

    const auto failure = Status::failure(ErrorCode::Parse, "invalid scene header");
    STORM_CHECK(!failure.ok());
    STORM_CHECK(!static_cast<bool>(failure));
    STORM_CHECK(failure.code() == ErrorCode::Parse);
    STORM_CHECK(failure.message() == "invalid scene header");

    // Guard against accidentally constructing a failed operation with the success code.
    const auto normalized_failure = Status::failure(ErrorCode::None, "invalid error code");
    STORM_CHECK(!normalized_failure.ok());
    STORM_CHECK(normalized_failure.code() == ErrorCode::Internal);
    STORM_CHECK(normalized_failure.message() == "invalid error code");
}

void test_runtime_lifecycle_normal_path() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;
    using storm::core::runtime_state_name;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(runtime_state_name(RuntimeState::Uninitialized) == "Uninitialized");
    STORM_CHECK(runtime_state_name(RuntimeState::Running) == "Running");
    STORM_CHECK(runtime_state_name(static_cast<RuntimeState>(255)) == "Unknown");

    const auto start_too_early = lifecycle.start();
    STORM_CHECK(!start_too_early.ok());
    STORM_CHECK(start_too_early.code() == ErrorCode::InvalidState);
    STORM_CHECK(start_too_early.message().find("Uninitialized") != std::string::npos);
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);

    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Initialized);
    STORM_CHECK(lifecycle.initialize().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Initialized);

    STORM_CHECK(lifecycle.start().ok());
    STORM_CHECK(lifecycle.is_running());
    STORM_CHECK(lifecycle.pause().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Paused);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(lifecycle.resume().ok());
    STORM_CHECK(lifecycle.is_running());

    STORM_CHECK(lifecycle.request_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopping);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(lifecycle.finish_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopped);
    STORM_CHECK(lifecycle.finish_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopped);

    STORM_CHECK(lifecycle.reset().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.failure_reason().has_value());
}

void test_runtime_lifecycle_invalid_transitions() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.pause().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.resume().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.request_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.finish_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.reset().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);

    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.request_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopping);
    STORM_CHECK(lifecycle.start().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.pause().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.reset().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.finish_stop().ok());
    STORM_CHECK(lifecycle.reset().ok());
}

void test_runtime_lifecycle_failure_and_recovery() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.start().ok());

    const auto fatal = lifecycle.fail(ErrorCode::Io, "platform initialization failed");
    STORM_CHECK(!fatal.ok());
    STORM_CHECK(fatal.code() == ErrorCode::Io);
    STORM_CHECK(fatal.message() == "platform initialization failed");
    STORM_CHECK(lifecycle.state() == RuntimeState::Failed);
    STORM_CHECK(lifecycle.failure_reason().has_value());
    if (lifecycle.failure_reason().has_value()) {
        STORM_CHECK(lifecycle.failure_reason()->code() == ErrorCode::Io);
        STORM_CHECK(lifecycle.failure_reason()->message() == "platform initialization failed");
    }

    STORM_CHECK(lifecycle.start().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.fail(ErrorCode::Internal, "second failure").code() ==
                ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.failure_reason()->message() == "platform initialization failed");

    STORM_CHECK(lifecycle.reset().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.failure_reason().has_value());
    STORM_CHECK(lifecycle.initialize().ok());

    const auto missing_code = lifecycle.fail(ErrorCode::None, "failure without code");
    STORM_CHECK(missing_code.code() == ErrorCode::Internal);
    STORM_CHECK(lifecycle.state() == RuntimeState::Failed);
}

void test_vector_arithmetic() {
    using storm::math::Vec1;
    using storm::math::Vec2;
    using storm::math::Vec3;

    STORM_CHECK(((Vec1{2.0F} + Vec1{3.0F}) == Vec1{5.0F}));
    STORM_CHECK(((Vec2{1.0F, 2.0F} + Vec2{3.0F, 4.0F}) == Vec2{4.0F, 6.0F}));
    STORM_CHECK(((Vec3{3.0F, 4.0F, 5.0F} - Vec3{1.0F, 2.0F, 3.0F}) ==
                 Vec3{2.0F, 2.0F, 2.0F}));
    STORM_CHECK((Vec2{3.0F, 4.0F}.dot(Vec2{3.0F, 4.0F}) == 25.0F));
    STORM_CHECK(((Vec3{1.0F, 2.0F, 3.0F} * 2.0F) == Vec3{2.0F, 4.0F, 6.0F}));
    STORM_CHECK(((2.0F * Vec2{2.0F, 5.0F}) == Vec2{4.0F, 10.0F}));

    auto accumulated = Vec2{1.0F, 2.0F};
    accumulated += Vec2{2.0F, 3.0F};
    STORM_CHECK((accumulated == Vec2{3.0F, 5.0F}));
    accumulated -= Vec2{1.0F, 1.0F};
    STORM_CHECK((accumulated == Vec2{2.0F, 4.0F}));
}

void test_vector_normalization_and_validation() {
    using storm::math::Vec2;

    const auto normalized = Vec2{3.0F, 4.0F}.normalized();
    STORM_CHECK(normalized.has_value());
    if (normalized.has_value()) {
        STORM_CHECK(std::abs(normalized->length() - 1.0F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](0) - 0.6F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](1) - 0.8F) < 1.0e-5F);
    }

    STORM_CHECK((!Vec2{0.0F, 0.0F}.normalized().has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(-1.0F).has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(
                     std::numeric_limits<float>::quiet_NaN()).has_value()));
    STORM_CHECK((!Vec2{std::numeric_limits<float>::infinity(), 1.0F}.is_finite()));
    STORM_CHECK((Vec2{1.0F, 1.0F}.is_finite()));
}

void test_dimensions() {
    using storm::scene::Dimension;
    using storm::scene::component_count;
    using storm::scene::dimension_name;
    using storm::scene::is_valid_dimension;

    STORM_CHECK(is_valid_dimension(Dimension::One));
    STORM_CHECK(is_valid_dimension(Dimension::Two));
    STORM_CHECK(is_valid_dimension(Dimension::Three));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(0)));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(4)));
    STORM_CHECK(component_count(Dimension::One) == 1U);
    STORM_CHECK(component_count(Dimension::Two) == 2U);
    STORM_CHECK(component_count(Dimension::Three) == 3U);
    STORM_CHECK(component_count(static_cast<Dimension>(255)) == 0U);
    STORM_CHECK(dimension_name(Dimension::One) == "1D");
    STORM_CHECK(dimension_name(Dimension::Two) == "2D");
    STORM_CHECK(dimension_name(Dimension::Three) == "3D");
    STORM_CHECK(dimension_name(static_cast<Dimension>(99)) == "Unknown");
}

void test_fixed_step_clock_basic_timing() {
    using storm::core::FixedStepClock;

    FixedStepClock clock;
    const auto first = clock.advance(1.0 / 60.0);
    STORM_CHECK(first.accepted);
    STORM_CHECK(first.steps == 1U);
    STORM_CHECK(nearly_equal(first.simulated_seconds, 1.0 / 60.0));
    STORM_CHECK(nearly_equal(first.interpolation_alpha, 0.0));

    const auto half_frame = clock.advance(1.0 / 120.0);
    STORM_CHECK(half_frame.accepted);
    STORM_CHECK(half_frame.steps == 0U);
    STORM_CHECK(nearly_equal(half_frame.interpolation_alpha, 0.5));

    const auto accumulated = clock.advance(1.0 / 120.0);
    STORM_CHECK(accumulated.steps == 1U);
    STORM_CHECK(nearly_equal(accumulated.interpolation_alpha, 0.0));
}

void test_fixed_step_clock_rounding_and_catch_up() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.5, 8U})));

    const auto three_ticks = clock.advance(0.3);
    STORM_CHECK(three_ticks.accepted);
    STORM_CHECK(three_ticks.steps == 3U);
    STORM_CHECK(nearly_equal(three_ticks.simulated_seconds, 0.3));
    STORM_CHECK(nearly_equal(three_ticks.discarded_seconds, 0.0));
    STORM_CHECK(nearly_equal(three_ticks.interpolation_alpha, 0.0));

    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.25, 8U})));
    const auto clamped = clock.advance(1.0);
    STORM_CHECK(clamped.accepted);
    STORM_CHECK(clamped.steps == 2U);
    STORM_CHECK(nearly_equal(clamped.simulated_seconds, 0.2));
    STORM_CHECK(nearly_equal(clamped.discarded_seconds, 0.75));
    STORM_CHECK(nearly_equal(clamped.interpolation_alpha, 0.5));
}

void test_fixed_step_clock_step_budget() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.8, 2U})));

    const auto bounded = clock.advance(0.65);
    STORM_CHECK(bounded.accepted);
    STORM_CHECK(bounded.steps == 2U);
    STORM_CHECK(nearly_equal(bounded.simulated_seconds, 0.2));
    STORM_CHECK(nearly_equal(bounded.discarded_seconds, 0.4));
    STORM_CHECK(nearly_equal(bounded.interpolation_alpha, 0.5));
    STORM_CHECK(clock.accumulator_seconds() >= 0.0);
    STORM_CHECK(clock.accumulator_seconds() < clock.config().fixed_delta_seconds);
}

void test_fixed_step_clock_rejects_invalid_time_and_configuration() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.5, 8U})));
    const auto partial = clock.advance(0.05);
    STORM_CHECK(partial.accepted);
    const double saved_accumulator = clock.accumulator_seconds();
    const auto saved_config = clock.config();

    const auto negative = clock.advance(-1.0);
    const auto nan = clock.advance(std::numeric_limits<double>::quiet_NaN());
    const auto infinity = clock.advance(std::numeric_limits<double>::infinity());
    STORM_CHECK(!negative.accepted);
    STORM_CHECK(!nan.accepted);
    STORM_CHECK(!infinity.accepted);
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), saved_accumulator));
    STORM_CHECK(nearly_equal(negative.interpolation_alpha, 0.5));

    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.0, 0.5, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.05, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.5, 0U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.5, 1025U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{
                     std::numeric_limits<double>::quiet_NaN(), 0.5, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 61.0, 8U})));

    STORM_CHECK(clock.config().fixed_delta_seconds == saved_config.fixed_delta_seconds);
    STORM_CHECK(clock.config().max_frame_delta_seconds == saved_config.max_frame_delta_seconds);
    STORM_CHECK(clock.config().max_steps_per_frame == saved_config.max_steps_per_frame);
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), saved_accumulator));

    STORM_CHECK((clock.configure(FixedStepClockConfig{0.2, 0.4, 4U})));
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), 0.0));
    const auto zero = clock.advance(0.0);
    STORM_CHECK(zero.accepted);
    STORM_CHECK(zero.steps == 0U);
    STORM_CHECK(nearly_equal(zero.interpolation_alpha, 0.0));
    clock.reset();
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), 0.0));
}

} // namespace

int main() {
    test_version();
    test_status_values();
    test_runtime_lifecycle_normal_path();
    test_runtime_lifecycle_invalid_transitions();
    test_runtime_lifecycle_failure_and_recovery();
    test_vector_arithmetic();
    test_vector_normalization_and_validation();
    test_dimensions();
    test_fixed_step_clock_basic_timing();
    test_fixed_step_clock_rounding_and_catch_up();
    test_fixed_step_clock_step_budget();
    test_fixed_step_clock_rejects_invalid_time_and_configuration();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All StormEngine core tests passed.\n";
    return 0;
}
