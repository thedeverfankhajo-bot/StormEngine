#include "storm/input/touch_input.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {
using Storm::Input::PointerEvent;
using Storm::Input::PointerPhase;
using Storm::Input::TouchInput;
using Storm::Input::TouchInputConfig;

#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

PointerEvent event(std::uint64_t id, PointerPhase phase, double x, double y,
                   std::uint64_t time, std::uint64_t generation = 1U) {
    return PointerEvent{id, phase, x, y, time, generation};
}

std::optional<std::string> configuration_limits() {
    EXPECT(!TouchInput::create(TouchInputConfig{0U, 1U}));
    EXPECT(!TouchInput::create(TouchInputConfig{65U, 65U}));
    EXPECT(!TouchInput::create(TouchInputConfig{2U, 1U}));
    EXPECT(!TouchInput::create(TouchInputConfig{
        1U, TouchInput::maximum_supported_events_per_frame + 1U}));
    EXPECT(TouchInput::create());
    return std::nullopt;
}
std::optional<std::string> surface_generation() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value();
    EXPECT(!input.process(event(1U, PointerPhase::Down, 1.0, 2.0, 1U)));
    EXPECT(input.attach_surface(1U));
    auto stale = input.process(event(1U, PointerPhase::Down, 1.0, 2.0, 2U, 2U));
    EXPECT(!stale);
    EXPECT(stale.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(input.surface_generation() == std::optional<std::uint64_t>{1U});
    return std::nullopt;
}
std::optional<std::string> down_move_up() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    input.begin_frame();
    EXPECT(input.process(event(7U, PointerPhase::Down, 10, 15, 100)));
    EXPECT(input.process(event(7U, PointerPhase::Move, 13, 9, 110)));
    EXPECT(input.process(event(7U, PointerPhase::Move, 15, 8, 120)));
    EXPECT(input.active_pointers()[0].delta_x == 5.0);
    EXPECT(input.active_pointers()[0].delta_y == -7.0);
    EXPECT(input.process(event(7U, PointerPhase::Up, 16, 8, 130)));
    EXPECT(input.active_pointer_count() == 0U);
    EXPECT(input.frame_events().size() == 4U);
    EXPECT(input.frame_events().back().phase == PointerPhase::Up);
    return std::nullopt;
}
std::optional<std::string> multi_touch_limit() {
    auto r = TouchInput::create(TouchInputConfig{2U, 4U}); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(1U, PointerPhase::Down, 1, 1, 1)));
    EXPECT(input.process(event(2U, PointerPhase::Down, 2, 2, 2)));
    auto third = input.process(event(3U, PointerPhase::Down, 3, 3, 3));
    EXPECT(!third); EXPECT(third.error().code == Storm::ErrorCode::ResourceExhausted);
    EXPECT(input.active_pointer_count() == 2U);
    return std::nullopt;
}
std::optional<std::string> invalid_transitions() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(1U, PointerPhase::Down, 0, 0, 1)));
    EXPECT(!input.process(event(1U, PointerPhase::Down, 1, 1, 2)));
    EXPECT(!input.process(event(2U, PointerPhase::Move, 1, 1, 2)));
    EXPECT(!input.process(event(3U, PointerPhase::Up, 1, 1, 2)));
    EXPECT(input.active_pointer_count() == 1U);
    EXPECT(input.frame_events().size() == 1U);
    return std::nullopt;
}
std::optional<std::string> non_finite_inputs() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    auto nan = input.process(event(1U, PointerPhase::Down,
        std::numeric_limits<double>::quiet_NaN(), 0, 1));
    EXPECT(!nan); EXPECT(input.active_pointer_count() == 0U);
    EXPECT(input.process(event(1U, PointerPhase::Down, 0, 0, 2)));
    auto inf = input.process(event(1U, PointerPhase::Move,
        0, std::numeric_limits<double>::infinity(), 3));
    EXPECT(!inf); EXPECT(input.active_pointers()[0].y == 0.0);
    EXPECT(input.active_pointers()[0].timestamp_ns == 2U);
    return std::nullopt;
}
std::optional<std::string> monotonic_timestamps() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(1U, PointerPhase::Down, 0, 0, 100)));
    EXPECT(!input.process(event(2U, PointerPhase::Down, 1, 1, 99)));
    EXPECT(!input.process(event(1U, PointerPhase::Move, 2, 2, 98)));
    EXPECT(input.active_pointers()[0].x == 0.0);
    return std::nullopt;
}
std::optional<std::string> detach_cancels_and_recreation_rejects_stale() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(5U, PointerPhase::Down, 4, 4, 10)));
    EXPECT(input.detach_surface(1U, 11U));
    EXPECT(input.active_pointer_count() == 0U);
    EXPECT(input.frame_events().back().phase == PointerPhase::Cancel);
    EXPECT(input.attach_surface(2U));
    auto stale = input.process(event(5U, PointerPhase::Move, 5, 4, 12, 1U));
    EXPECT(!stale);
    EXPECT(input.active_pointer_count() == 0U);
    return std::nullopt;
}
std::optional<std::string> cancel_and_reuse_id() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(9U, PointerPhase::Down, 1, 1, 10)));
    EXPECT(input.cancel_all(20U));
    EXPECT(input.frame_events().back().phase == PointerPhase::Cancel);
    EXPECT(input.process(event(9U, PointerPhase::Down, 2, 2, 21)));
    EXPECT(input.active_pointer_count() == 1U);
    return std::nullopt;
}
std::optional<std::string> frame_reset() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(4U, PointerPhase::Down, 2, 3, 1)));
    EXPECT(input.process(event(4U, PointerPhase::Move, 5, 7, 2)));
    EXPECT(input.active_pointers()[0].delta_x == 3.0);
    input.begin_frame();
    EXPECT(input.frame_events().empty());
    EXPECT(input.active_pointer_count() == 1U);
    EXPECT(input.active_pointers()[0].delta_x == 0.0);
    EXPECT(input.process(event(4U, PointerPhase::Move, 6, 9, 3)));
    EXPECT(input.active_pointers()[0].delta_x == 1.0);
    return std::nullopt;
}
std::optional<std::string> event_capacity_is_transactional() {
    auto r = TouchInput::create(TouchInputConfig{1U, 2U}); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(1U, PointerPhase::Down, 0, 0, 1)));
    EXPECT(input.process(event(1U, PointerPhase::Move, 1, 0, 2)));
    auto full = input.process(event(1U, PointerPhase::Move, 2, 0, 3));
    EXPECT(!full); EXPECT(full.error().code == Storm::ErrorCode::ResourceExhausted);
    EXPECT(input.active_pointers()[0].x == 1.0);
    input.begin_frame();
    EXPECT(input.process(event(1U, PointerPhase::Move, 2, 0, 3)));
    return std::nullopt;
}
std::optional<std::string> cancellation_capacity_is_transactional() {
    auto r = TouchInput::create(TouchInputConfig{1U, 1U}); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    EXPECT(input.process(event(1U, PointerPhase::Down, 1, 2, 1)));
    auto full = input.cancel_all(2U);
    EXPECT(!full); EXPECT(input.active_pointer_count() == 1U);
    input.begin_frame(); EXPECT(input.cancel_all(2U));
    EXPECT(input.active_pointer_count() == 0U);
    return std::nullopt;
}
std::optional<std::string> invalid_phase_rejected() {
    auto r = TouchInput::create(); EXPECT(r);
    auto input = std::move(r).value(); EXPECT(input.attach_surface(1U));
    auto invalid = input.process(event(
        1U, static_cast<PointerPhase>(255U), 0, 0, 1));
    EXPECT(!invalid); EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(input.frame_events().empty());
    return std::nullopt;
}
std::optional<std::string> moved_from_is_inert() {
    auto r = TouchInput::create(); EXPECT(r);
    auto original = std::move(r).value(); EXPECT(original.attach_surface(1U));
    TouchInput moved(std::move(original));
    auto old_result = original.attach_surface(2U);
    auto moved_result = moved.attach_surface(2U);
    EXPECT(!old_result); EXPECT(!moved_result);
    EXPECT(old_result.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(moved_result.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

struct TestCase { const char* name; std::optional<std::string> (*run)(); };
} // namespace

int main() {
    constexpr std::array cases{
        TestCase{"configuration_limits", &configuration_limits},
        TestCase{"surface_generation", &surface_generation},
        TestCase{"down_move_up", &down_move_up},
        TestCase{"multi_touch_limit", &multi_touch_limit},
        TestCase{"invalid_transitions", &invalid_transitions},
        TestCase{"non_finite_inputs", &non_finite_inputs},
        TestCase{"monotonic_timestamps", &monotonic_timestamps},
        TestCase{"detach_and_stale_generation", &detach_cancels_and_recreation_rejects_stale},
        TestCase{"cancel_and_id_reuse", &cancel_and_reuse_id},
        TestCase{"frame_reset", &frame_reset},
        TestCase{"event_capacity_transaction", &event_capacity_is_transactional},
        TestCase{"cancel_capacity_transaction", &cancellation_capacity_is_transactional},
        TestCase{"invalid_phase", &invalid_phase_rejected},
        TestCase{"moved_from_inert", &moved_from_is_inert},
    };
    std::size_t failures = 0U;
    for (const auto& item : cases) {
        const auto failure = item.run();
        if (failure) {
            ++failures;
            std::cerr << "[FAIL] " << item.name << ": " << *failure << '\n';
        } else {
            std::cout << "[PASS] " << item.name << '\n';
        }
    }
    std::cout << "Executed " << cases.size() << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
