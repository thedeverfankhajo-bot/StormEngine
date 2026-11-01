#include "storm/platform/ios/lifecycle_adapter.hpp"

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
using Storm::Platform::IOS::ApplicationEvent;
using Storm::Platform::IOS::SceneEvent;
using Storm::Platform::IOS::apply_application_event;
using Storm::Platform::IOS::apply_scene_event;

[[nodiscard]] std::optional<std::string> test_foreground_background_sequence() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillEnterForeground));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    EXPECT(!lifecycle.can_run_game_loop());

    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidBecomeActive));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);
    EXPECT(lifecycle.can_run_game_loop());

    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillResignActive));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    EXPECT(!lifecycle.can_run_game_loop());

    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidEnterBackground));
    EXPECT(lifecycle.state() == LifecycleState::Background);
    EXPECT(!lifecycle.is_foreground());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_active_callback_recovers_from_background() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidEnterBackground));
    EXPECT(lifecycle.state() == LifecycleState::Background);

    // Be resilient if the adapter attaches after the scene's foreground
    // transition notification and only observes the active callback.
    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidBecomeActive));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);
    EXPECT(lifecycle.can_run_game_loop());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_background_from_created_and_repeated_background() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidEnterBackground));
    EXPECT(lifecycle.state() == LifecycleState::Background);
    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidEnterBackground));
    EXPECT(lifecycle.state() == LifecycleState::Background);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_repeated_scene_events_are_idempotent() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillEnterForeground));
    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillEnterForeground));
    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidBecomeActive));
    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidBecomeActive));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundActive);

    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillResignActive));
    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillResignActive));
    EXPECT(lifecycle.state() == LifecycleState::ForegroundInactive);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_scene_lifecycles_are_independent() {
    LifecycleStateMachine first_scene;
    LifecycleStateMachine second_scene;

    EXPECT(apply_scene_event(first_scene, SceneEvent::WillEnterForeground));
    EXPECT(apply_scene_event(first_scene, SceneEvent::DidBecomeActive));
    EXPECT(apply_scene_event(second_scene, SceneEvent::DidEnterBackground));

    EXPECT(first_scene.state() == LifecycleState::ForegroundActive);
    EXPECT(second_scene.state() == LifecycleState::Background);
    EXPECT(first_scene.can_run_game_loop());
    EXPECT(!second_scene.can_run_game_loop());

    EXPECT(apply_scene_event(second_scene, SceneEvent::WillEnterForeground));
    EXPECT(second_scene.state() == LifecycleState::ForegroundInactive);
    EXPECT(first_scene.state() == LifecycleState::ForegroundActive);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_application_termination_is_terminal() {
    LifecycleStateMachine lifecycle;

    EXPECT(apply_scene_event(lifecycle, SceneEvent::WillEnterForeground));
    EXPECT(apply_scene_event(lifecycle, SceneEvent::DidBecomeActive));
    EXPECT(apply_application_event(lifecycle, ApplicationEvent::WillTerminate));
    EXPECT(lifecycle.is_terminating());

    EXPECT(apply_application_event(lifecycle, ApplicationEvent::WillTerminate));
    const auto late_scene_event =
        apply_scene_event(lifecycle, SceneEvent::DidBecomeActive);
    EXPECT(!late_scene_event);
    EXPECT(late_scene_event.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(lifecycle.is_terminating());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_unknown_events_are_rejected() {
    LifecycleStateMachine lifecycle;

    const auto unknown_scene = apply_scene_event(
        lifecycle, static_cast<SceneEvent>(255));
    EXPECT(!unknown_scene);
    EXPECT(unknown_scene.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(lifecycle.state() == LifecycleState::Created);

    const auto unknown_application = apply_application_event(
        lifecycle, static_cast<ApplicationEvent>(255));
    EXPECT(!unknown_application);
    EXPECT(unknown_application.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(lifecycle.state() == LifecycleState::Created);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr TestCase tests[]{
        {"foreground_background_sequence", &test_foreground_background_sequence},
        {"active_callback_recovers_from_background", &test_active_callback_recovers_from_background},
        {"background_from_created_and_repeated_background", &test_background_from_created_and_repeated_background},
        {"repeated_scene_events_are_idempotent", &test_repeated_scene_events_are_idempotent},
        {"scene_lifecycles_are_independent", &test_scene_lifecycles_are_independent},
        {"application_termination_is_terminal", &test_application_termination_is_terminal},
        {"unknown_events_are_rejected", &test_unknown_events_are_rejected},
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
