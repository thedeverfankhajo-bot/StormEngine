#include "storm/input/gesture_recognizer.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {
using Storm::Input::GestureEvent;
using Storm::Input::GestureRecognizer;
using Storm::Input::GestureRecognizerConfig;
using Storm::Input::GestureType;
using Storm::Input::PointerEvent;
using Storm::Input::PointerPhase;

#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

PointerEvent event(
    std::uint64_t id,
    PointerPhase phase,
    double x,
    double y,
    std::uint64_t timestamp,
    std::uint64_t generation = 1U) {
    return PointerEvent{id, phase, x, y, timestamp, generation};
}

std::optional<std::string> configuration_validation() {
    EXPECT(!GestureRecognizer::create(GestureRecognizerConfig{
        .movement_threshold = 0.0}));
    EXPECT(!GestureRecognizer::create(GestureRecognizerConfig{
        .double_tap_distance = -1.0}));
    EXPECT(!GestureRecognizer::create(GestureRecognizerConfig{
        .maximum_gesture_events_per_frame = 0U}));
    EXPECT(!GestureRecognizer::create(GestureRecognizerConfig{
        .maximum_gesture_events_per_frame =
            GestureRecognizer::maximum_supported_gesture_events_per_frame + 1U}));
    EXPECT(GestureRecognizer::create());
    return std::nullopt;
}

std::optional<std::string> tap_and_double_tap() {
    auto created = GestureRecognizer::create();
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent first_down = event(1U, PointerPhase::Down, 10.0, 20.0, 100U);
    auto frame = recognizer.update(
        std::span<const PointerEvent>{&first_down, 1U}, 100U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().empty());

    const PointerEvent first_up = event(1U, PointerPhase::Up, 10.0, 20.0, 150U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&first_up, 1U}, 150U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 1U);
    EXPECT(frame.value()[0].type == GestureType::Tap);

    const PointerEvent second_down = event(2U, PointerPhase::Down, 12.0, 21.0, 200U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&second_down, 1U}, 200U, 1U);
    EXPECT(frame);

    const PointerEvent second_up = event(2U, PointerPhase::Up, 12.0, 21.0, 250U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&second_up, 1U}, 250U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::Tap);
    EXPECT(frame.value()[1].type == GestureType::DoubleTap);
    return std::nullopt;
}

std::optional<std::string> long_press_without_move() {
    auto created = GestureRecognizer::create();
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent down = event(3U, PointerPhase::Down, 5.0, 7.0, 100U);
    auto frame = recognizer.update(
        std::span<const PointerEvent>{&down, 1U}, 100U, 1U);
    EXPECT(frame);

    frame = recognizer.update({}, 500'000'100U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 1U);
    EXPECT(frame.value()[0].type == GestureType::LongPress);

    const PointerEvent up = event(
        3U, PointerPhase::Up, 5.0, 7.0, 600'000'100U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&up, 1U}, 600'000'100U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().empty());
    return std::nullopt;
}

std::optional<std::string> drag_and_swipe() {
    GestureRecognizerConfig config{};
    config.minimum_swipe_velocity = 500.0;
    auto created = GestureRecognizer::create(config);
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent down = event(4U, PointerPhase::Down, 10.0, 10.0, 0U);
    EXPECT(recognizer.update(
        std::span<const PointerEvent>{&down, 1U}, 0U, 1U));

    const PointerEvent move = event(4U, PointerPhase::Move, 30.0, 10.0, 100'000'000U);
    auto frame = recognizer.update(
        std::span<const PointerEvent>{&move, 1U}, 100'000'000U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::DragBegin);
    EXPECT(frame.value()[1].type == GestureType::DragUpdate);
    EXPECT(frame.value()[1].delta_x == 20.0);

    const PointerEvent up = event(4U, PointerPhase::Up, 130.0, 10.0, 200'000'000U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&up, 1U}, 200'000'000U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::DragEnd);
    EXPECT(frame.value()[1].type == GestureType::Swipe);
    EXPECT(std::abs(frame.value()[1].velocity_x - 600.0) < 0.001);
    return std::nullopt;
}

std::optional<std::string> pinch_and_two_pointer_pan() {
    auto created = GestureRecognizer::create();
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent first_down = event(10U, PointerPhase::Down, 0.0, 0.0, 1U);
    EXPECT(recognizer.update(
        std::span<const PointerEvent>{&first_down, 1U}, 1U, 1U));

    const PointerEvent second_down = event(11U, PointerPhase::Down, 10.0, 0.0, 2U);
    auto frame = recognizer.update(
        std::span<const PointerEvent>{&second_down, 1U}, 2U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::PanBegin);
    EXPECT(frame.value()[1].type == GestureType::PinchBegin);

    const PointerEvent move = event(11U, PointerPhase::Move, 20.0, 0.0, 3U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&move, 1U}, 3U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::PanUpdate);
    EXPECT(frame.value()[1].type == GestureType::PinchUpdate);
    EXPECT(std::abs(frame.value()[1].scale - 2.0) < 0.001);

    const PointerEvent up = event(11U, PointerPhase::Up, 20.0, 0.0, 4U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&up, 1U}, 4U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 4U);
    EXPECT(frame.value()[2].type == GestureType::PinchEnd);
    EXPECT(frame.value()[3].type == GestureType::PanEnd);
    EXPECT(!frame.value()[2].cancelled);
    return std::nullopt;
}

std::optional<std::string> cancellation_and_surface_generation() {
    auto created = GestureRecognizer::create();
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent down = event(20U, PointerPhase::Down, 2.0, 3.0, 10U);
    EXPECT(recognizer.update(
        std::span<const PointerEvent>{&down, 1U}, 10U, 1U));

    auto frame = recognizer.update({}, 20U, 2U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 1U);
    EXPECT(frame.value()[0].type == GestureType::Cancelled);
    EXPECT(frame.value()[0].cancelled);
    EXPECT(frame.value()[0].pointer_id == 20U);

    const PointerEvent stale = event(21U, PointerPhase::Down, 0.0, 0.0, 30U, 1U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&stale, 1U}, 30U, 2U);
    EXPECT(!frame);
    EXPECT(frame.error().code == Storm::ErrorCode::InvalidState);
    return std::nullopt;
}

std::optional<std::string> bounded_output_and_monotonic_validation() {
    GestureRecognizerConfig config{};
    config.maximum_gesture_events_per_frame = 1U;
    auto created = GestureRecognizer::create(config);
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const std::array two_downs{
        event(30U, PointerPhase::Down, 0.0, 0.0, 1U),
        event(31U, PointerPhase::Down, 10.0, 0.0, 2U),
    };
    auto frame = recognizer.update(two_downs, 2U, 1U);
    EXPECT(!frame);
    EXPECT(frame.error().code == Storm::ErrorCode::ResourceExhausted);

    auto normal = GestureRecognizer::create();
    EXPECT(normal);
    auto monotonic = std::move(normal).value();
    const PointerEvent down = event(40U, PointerPhase::Down, 0.0, 0.0, 100U);
    EXPECT(monotonic.update(
        std::span<const PointerEvent>{&down, 1U}, 100U, 1U));
    frame = monotonic.update({}, 99U, 1U);
    EXPECT(!frame);
    EXPECT(frame.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}


std::optional<std::string> drag_ends_when_pinch_begins() {
    GestureRecognizerConfig config{};
    config.movement_threshold = 5.0;
    auto created = GestureRecognizer::create(config);
    EXPECT(created);
    auto recognizer = std::move(created).value();

    const PointerEvent first_down = event(50U, PointerPhase::Down, 0.0, 0.0, 1U);
    EXPECT(recognizer.update(
        std::span<const PointerEvent>{&first_down, 1U}, 1U, 1U));

    const PointerEvent first_move = event(50U, PointerPhase::Move, 10.0, 0.0, 2U);
    auto frame = recognizer.update(
        std::span<const PointerEvent>{&first_move, 1U}, 2U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 2U);
    EXPECT(frame.value()[0].type == GestureType::DragBegin);
    EXPECT(frame.value()[1].type == GestureType::DragUpdate);

    const PointerEvent second_down = event(51U, PointerPhase::Down, 30.0, 0.0, 3U);
    frame = recognizer.update(
        std::span<const PointerEvent>{&second_down, 1U}, 3U, 1U);
    EXPECT(frame);
    EXPECT(frame.value().size() == 3U);
    EXPECT(frame.value()[0].type == GestureType::DragEnd);
    EXPECT(frame.value()[0].pointer_id == 50U);
    EXPECT(frame.value()[1].type == GestureType::PanBegin);
    EXPECT(frame.value()[2].type == GestureType::PinchBegin);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"configuration_validation", &configuration_validation},
        TestCase{"tap_and_double_tap", &tap_and_double_tap},
        TestCase{"long_press_without_move", &long_press_without_move},
        TestCase{"drag_and_swipe", &drag_and_swipe},
        TestCase{"pinch_and_two_pointer_pan", &pinch_and_two_pointer_pan},
        TestCase{"cancellation_and_surface_generation", &cancellation_and_surface_generation},
        TestCase{"bounded_output_and_monotonic_validation",
                 &bounded_output_and_monotonic_validation},
        TestCase{"drag_ends_when_pinch_begins", &drag_ends_when_pinch_begins},
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
