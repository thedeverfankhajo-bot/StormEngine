#include "storm/runtime/mobile_frame_driver.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;
using Driver = Storm::Runtime::MobileFrameDriver;
using Disposition = Storm::Runtime::FrameDisposition;

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

class RecordingCallbacks final : public Storm::IGameLoopCallbacks {
public:
    [[nodiscard]] Storm::Result<void> process_input() override {
        ++input_count;
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> process_events() override {
        ++event_count;
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> fixed_update(
        std::chrono::nanoseconds fixed_delta) override {
        fixed_deltas.push_back(fixed_delta);
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> variable_update(
        std::chrono::nanoseconds accepted_frame_delta) override {
        variable_delta = accepted_frame_delta;
        if (fail_variable_update) {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure,
                "injected variable update failure"));
        }
        return Storm::Result<void>::success();
    }

    [[nodiscard]] Storm::Result<void> render(double interpolation_alpha) override {
        ++render_count;
        render_alpha = interpolation_alpha;
        return Storm::Result<void>::success();
    }

    void clear_counts() {
        input_count = 0U;
        event_count = 0U;
        fixed_deltas.clear();
        render_count = 0U;
        variable_delta = std::chrono::nanoseconds{0};
        render_alpha = 0.0;
    }

    std::size_t input_count{0U};
    std::size_t event_count{0U};
    std::vector<std::chrono::nanoseconds> fixed_deltas;
    std::size_t render_count{0U};
    std::chrono::nanoseconds variable_delta{0};
    double render_alpha{0.0};
    bool fail_variable_update{false};
};

[[nodiscard]] bool make_active(Storm::Platform::LifecycleStateMachine& lifecycle) {
    const auto inactive = lifecycle.transition_to(
        Storm::Platform::LifecycleState::ForegroundInactive);
    if (!inactive) {
        return false;
    }
    const auto active = lifecycle.transition_to(
        Storm::Platform::LifecycleState::ForegroundActive);
    return static_cast<bool>(active);
}

[[nodiscard]] Storm::FixedStepConfig test_config() {
    return Storm::FixedStepConfig{
        .fixed_step = 10ms,
        .maximum_frame_delta = 100ms,
        .maximum_steps_per_frame = 8U,
    };
}

[[nodiscard]] std::optional<std::string> test_rejects_invalid_loop_configuration() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    auto created = Driver::create(
        Storm::FixedStepConfig{.maximum_steps_per_frame = 0U},
        lifecycle,
        surface);
    EXPECT(!created);
    EXPECT(created.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_inactive_lifecycle_skips_every_game_callback() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;

    const auto result = driver.tick(Driver::TimePoint{}, callbacks);
    EXPECT(result);
    EXPECT(result.value().disposition == Disposition::SkippedInactive);
    EXPECT(callbacks.input_count == 0U);
    EXPECT(callbacks.event_count == 0U);
    EXPECT(callbacks.fixed_deltas.empty());
    EXPECT(callbacks.render_count == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_active_lifecycle_without_surface_skips_callbacks() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;

    const auto result = driver.tick(Driver::TimePoint{}, callbacks);
    EXPECT(result);
    EXPECT(result.value().disposition == Disposition::SkippedSurfaceUnavailable);
    EXPECT(callbacks.input_count == 0U);
    EXPECT(callbacks.render_count == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_first_eligible_frame_uses_zero_delta() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;

    const auto result = driver.tick(Driver::TimePoint{}, callbacks);
    EXPECT(result);
    EXPECT(result.value().disposition == Disposition::Executed);
    EXPECT(result.value().timing.step_count == 0U);
    EXPECT(callbacks.input_count == 1U);
    EXPECT(callbacks.event_count == 1U);
    EXPECT(callbacks.variable_delta == 0ns);
    EXPECT(callbacks.render_count == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_monotonic_delta_drives_bounded_fixed_updates() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start, callbacks));
    callbacks.clear_counts();
    const auto result = driver.tick(start + 25ms, callbacks);
    EXPECT(result);
    EXPECT(result.value().disposition == Disposition::Executed);
    EXPECT(result.value().timing.step_count == 2U);
    EXPECT(result.value().timing.interpolation_alpha == 0.5);
    EXPECT(callbacks.fixed_deltas.size() == 2U);
    EXPECT(callbacks.fixed_deltas[0] == 10ms);
    EXPECT(callbacks.fixed_deltas[1] == 10ms);
    EXPECT(callbacks.variable_delta == 25ms);
    EXPECT(callbacks.render_count == 1U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_lifecycle_revision_discards_time_between_frames() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start, callbacks));
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::ForegroundInactive));
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::Background));
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::ForegroundInactive));
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::ForegroundActive));

    const auto resumed = driver.tick(start + 5s, callbacks);
    EXPECT(resumed);
    EXPECT(resumed.value().disposition == Disposition::Executed);
    EXPECT(resumed.value().timing.step_count == 0U);
    EXPECT(callbacks.variable_delta == 0ns);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_surface_recreation_discards_time_between_generations() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    const auto first_surface = surface.attach(1U, {320U, 640U}, 1.0);
    EXPECT(first_surface);
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start, callbacks));
    EXPECT(surface.detach(first_surface.value().native_id, first_surface.value().generation));
    const auto replacement = surface.attach(2U, {640U, 320U}, 1.0);
    EXPECT(replacement);
    const auto recreated = driver.tick(start + 8s, callbacks);
    EXPECT(recreated);
    EXPECT(recreated.value().timing.step_count == 0U);
    EXPECT(callbacks.variable_delta == 0ns);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_backwards_timestamp_is_rejected_and_driver_recovers() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start + 10s, callbacks));
    callbacks.clear_counts();
    const auto invalid = driver.tick(start + 9s, callbacks);
    EXPECT(!invalid);
    EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(callbacks.input_count == 0U);
    const auto recovered = driver.tick(start + 11s, callbacks);
    EXPECT(recovered);
    EXPECT(recovered.value().timing.step_count == 0U);
    EXPECT(callbacks.variable_delta == 0ns);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_failed_frame_delta_is_not_replayed() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    EXPECT(surface.attach(1U, {320U, 640U}, 1.0));
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start, callbacks));
    callbacks.fail_variable_update = true;
    const auto failed = driver.tick(start + 20ms, callbacks);
    EXPECT(!failed);
    EXPECT(failed.error().code == Storm::ErrorCode::IoFailure);

    callbacks.fail_variable_update = false;
    callbacks.clear_counts();
    const auto next = driver.tick(start + 30ms, callbacks);
    EXPECT(next);
    EXPECT(next.value().timing.step_count == 1U);
    EXPECT(callbacks.variable_delta == 10ms);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_surface_resize_preserves_frame_timing() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    EXPECT(make_active(lifecycle));
    const auto attached = surface.attach(1U, {320U, 640U}, 1.0);
    EXPECT(attached);
    auto created = Driver::create(test_config(), lifecycle, surface);
    EXPECT(created);
    auto driver = std::move(created).value();
    RecordingCallbacks callbacks;
    const Driver::TimePoint start{};

    EXPECT(driver.tick(start, callbacks));
    EXPECT(surface.resize(
        attached.value().native_id,
        attached.value().generation,
        {640U, 320U},
        2.0));
    const auto after_resize = driver.tick(start + 25ms, callbacks);
    EXPECT(after_resize);
    EXPECT(after_resize.value().timing.step_count == 2U);
    EXPECT(callbacks.variable_delta == 25ms);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_lifecycle_revision_only_changes_for_real_transitions() {
    Storm::Platform::LifecycleStateMachine lifecycle;
    EXPECT(lifecycle.revision() == 0U);
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::ForegroundInactive));
    EXPECT(lifecycle.revision() == 1U);
    const auto duplicate = lifecycle.transition_to(
        Storm::Platform::LifecycleState::ForegroundInactive);
    EXPECT(duplicate);
    EXPECT(!duplicate.value().changed);
    EXPECT(lifecycle.revision() == 1U);
    EXPECT(lifecycle.transition_to(Storm::Platform::LifecycleState::ForegroundActive));
    EXPECT(lifecycle.revision() == 2U);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"rejects_invalid_loop_configuration", &test_rejects_invalid_loop_configuration},
        TestCase{"inactive_lifecycle_skips_every_game_callback", &test_inactive_lifecycle_skips_every_game_callback},
        TestCase{"active_lifecycle_without_surface_skips_callbacks", &test_active_lifecycle_without_surface_skips_callbacks},
        TestCase{"first_eligible_frame_uses_zero_delta", &test_first_eligible_frame_uses_zero_delta},
        TestCase{"monotonic_delta_drives_bounded_fixed_updates", &test_monotonic_delta_drives_bounded_fixed_updates},
        TestCase{"lifecycle_revision_discards_time_between_frames", &test_lifecycle_revision_discards_time_between_frames},
        TestCase{"surface_recreation_discards_time_between_generations", &test_surface_recreation_discards_time_between_generations},
        TestCase{"backwards_timestamp_is_rejected_and_driver_recovers", &test_backwards_timestamp_is_rejected_and_driver_recovers},
        TestCase{"failed_frame_delta_is_not_replayed", &test_failed_frame_delta_is_not_replayed},
        TestCase{"surface_resize_preserves_frame_timing", &test_surface_resize_preserves_frame_timing},
        TestCase{"lifecycle_revision_only_changes_for_real_transitions", &test_lifecycle_revision_only_changes_for_real_transitions},
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
