#include "storm/platform/android/lifecycle_adapter.hpp"

#include <cstdlib>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

using Storm::Platform::LifecycleState;
using Storm::Platform::LifecycleStateMachine;
using Storm::Platform::Android::ActivityEvent;
using Storm::Platform::Android::apply_activity_event;

[[nodiscard]] std::optional<std::string> test_resume_focus_pause_stop_sequence() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Started));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    EXPECT(!lifecycle.can_run_game_loop());

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Resumed));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusGained));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);
    EXPECT(lifecycle.can_run_game_loop());

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusLost));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    EXPECT(!lifecycle.can_run_game_loop());

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Paused));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Stopped));
    EXPECT(lifecycle.state() == LifecycleState::Background);
    EXPECT(!lifecycle.is_foreground());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_stop_from_active_state_is_ordered() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Resumed));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusGained));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);

    // STOP can be observed without a separate PAUSE/LOST_FOCUS callback.
    // The adapter explicitly passes through ForegroundInactive before Background.
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Stopped));
    EXPECT(lifecycle.state() == LifecycleState::Background);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_focus_gain_recovers_from_background() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Started));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Stopped));
    EXPECT(lifecycle.state() == LifecycleState::Background);

    // Focus gain is normalized through the inactive state if RESUME was not
    // delivered separately.
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusGained));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);
    EXPECT(lifecycle.can_run_game_loop());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_repeated_events_are_idempotent() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Resumed));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Resumed));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusGained));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusGained));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusLost));
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::FocusLost));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_destroy_is_terminal_and_unknown_events_fail() {
    LifecycleStateMachine lifecycle;

    const auto unknown = apply_activity_event(
        lifecycle, static_cast<ActivityEvent>(255));
    EXPECT(!unknown);
    EXPECT(unknown.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(lifecycle.state() == LifecycleState::Created);

    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Destroyed));
    EXPECT(lifecycle.is_terminating());
    EXPECT(apply_activity_event(lifecycle, ActivityEvent::Destroyed));

    const auto late_resume = apply_activity_event(lifecycle, ActivityEvent::Resumed);
    EXPECT(!late_resume);
    EXPECT(late_resume.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(lifecycle.is_terminating());
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr TestCase tests[]{
        {"resume_focus_pause_stop_sequence", &test_resume_focus_pause_stop_sequence},
        {"stop_from_active_state_is_ordered", &test_stop_from_active_state_is_ordered},
        {"focus_gain_recovers_from_background", &test_focus_gain_recovers_from_background},
        {"repeated_events_are_idempotent", &test_repeated_events_are_idempotent},
        {"destroy_is_terminal_and_unknown_events_fail", &test_destroy_is_terminal_and_unknown_events_fail},
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
