#include "storm/core/event_dispatcher.hpp"

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

struct ValueEvent final {
    int value{0};
};

struct OtherEvent final {
    int value{0};
};

[[nodiscard]] std::optional<std::string> test_dispatch_order_and_exact_event_type() {
    Storm::EventDispatcher dispatcher;
    std::vector<int> observed;

    auto first = dispatcher.subscribe<ValueEvent>(
        [&observed](const ValueEvent& event) {
            observed.push_back(event.value);
        });
    EXPECT(first);

    auto second = dispatcher.subscribe<ValueEvent>(
        [&observed](const ValueEvent& event) -> Storm::Result<void> {
            observed.push_back(event.value + 10);
            return Storm::Result<void>::success();
        });
    EXPECT(second);

    auto other = dispatcher.subscribe<OtherEvent>(
        [&observed](const OtherEvent& event) {
            observed.push_back(event.value + 100);
        });
    EXPECT(other);

    const auto result = dispatcher.dispatch(ValueEvent{.value = 5});
    EXPECT(result);
    EXPECT(result.value() == 2U);
    EXPECT(observed.size() == 2U);
    EXPECT(observed[0] == 5);
    EXPECT(observed[1] == 15);

    const auto other_result = dispatcher.dispatch(OtherEvent{.value = 3});
    EXPECT(other_result);
    EXPECT(other_result.value() == 1U);
    EXPECT(observed.size() == 3U);
    EXPECT(observed[2] == 103);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_dispatch_without_listeners_is_empty_success() {
    Storm::EventDispatcher dispatcher;
    const auto result = dispatcher.dispatch(ValueEvent{.value = 1});
    EXPECT(result);
    EXPECT(result.value() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_subscription_is_raii_and_move_only() {
    Storm::EventDispatcher dispatcher;
    int calls = 0;

    auto subscribed = dispatcher.subscribe<ValueEvent>(
        [&calls](const ValueEvent&) { ++calls; });
    EXPECT(subscribed);
    EXPECT(subscribed.value().connected());

    auto subscription = std::move(subscribed).value();
    EXPECT(subscription.connected());
    EXPECT(!subscribed.value().connected());

    auto first_dispatch = dispatcher.dispatch(ValueEvent{});
    EXPECT(first_dispatch);
    EXPECT(first_dispatch.value() == 1U);
    EXPECT(calls == 1);

    subscription.disconnect();
    EXPECT(!subscription.connected());

    auto second_dispatch = dispatcher.dispatch(ValueEvent{});
    EXPECT(second_dispatch);
    EXPECT(second_dispatch.value() == 0U);
    EXPECT(calls == 1);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_disconnect_during_dispatch_skips_pending_handler() {
    Storm::EventDispatcher dispatcher;
    std::vector<int> observed;
    std::optional<Storm::EventDispatcher::Subscription> second_subscription;

    auto first = dispatcher.subscribe<ValueEvent>(
        [&observed, &second_subscription](const ValueEvent&) {
            observed.push_back(1);
            second_subscription->disconnect();
        });
    EXPECT(first);

    auto second = dispatcher.subscribe<ValueEvent>(
        [&observed](const ValueEvent&) { observed.push_back(2); });
    EXPECT(second);
    second_subscription.emplace(std::move(second).value());

    const auto result = dispatcher.dispatch(ValueEvent{});
    EXPECT(result);
    EXPECT(result.value() == 1U);
    EXPECT(observed.size() == 1U);
    EXPECT(observed[0] == 1);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_new_subscription_starts_on_next_dispatch() {
    Storm::EventDispatcher dispatcher;
    std::vector<int> observed;
    std::optional<Storm::EventDispatcher::Subscription> late_subscription;
    bool registered = false;

    auto first = dispatcher.subscribe<ValueEvent>(
        [&dispatcher, &observed, &late_subscription, &registered](const ValueEvent&) {
            observed.push_back(1);
            if (!registered) {
                auto late = dispatcher.subscribe<ValueEvent>(
                    [&observed](const ValueEvent&) { observed.push_back(2); });
                if (late) {
                    late_subscription.emplace(std::move(late).value());
                    registered = true;
                }
            }
        });
    EXPECT(first);

    const auto first_dispatch = dispatcher.dispatch(ValueEvent{});
    EXPECT(first_dispatch);
    EXPECT(first_dispatch.value() == 1U);
    EXPECT(observed.size() == 1U);
    EXPECT(observed[0] == 1);

    const auto second_dispatch = dispatcher.dispatch(ValueEvent{});
    EXPECT(second_dispatch);
    EXPECT(second_dispatch.value() == 2U);
    EXPECT(observed.size() == 3U);
    EXPECT(observed[1] == 1);
    EXPECT(observed[2] == 2);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_clear_skips_pending_handlers() {
    Storm::EventDispatcher dispatcher;
    std::vector<int> observed;

    auto first = dispatcher.subscribe<ValueEvent>(
        [&dispatcher, &observed](const ValueEvent&) {
            observed.push_back(1);
            dispatcher.clear();
        });
    EXPECT(first);

    auto second = dispatcher.subscribe<ValueEvent>(
        [&observed](const ValueEvent&) { observed.push_back(2); });
    EXPECT(second);

    const auto result = dispatcher.dispatch(ValueEvent{});
    EXPECT(result);
    EXPECT(result.value() == 1U);
    EXPECT(observed.size() == 1U);
    EXPECT(observed[0] == 1);
    EXPECT(!first.value().connected());
    EXPECT(!second.value().connected());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_handler_failure_propagates_and_stops_dispatch() {
    Storm::EventDispatcher dispatcher;
    int later_handler_calls = 0;

    auto failing = dispatcher.subscribe<ValueEvent>(
        [](const ValueEvent&) -> Storm::Result<void> {
            return Storm::Result<void>::failure(Storm::Error::make(
                Storm::ErrorCode::IoFailure,
                "injected handler failure"));
        });
    EXPECT(failing);

    auto later = dispatcher.subscribe<ValueEvent>(
        [&later_handler_calls](const ValueEvent&) { ++later_handler_calls; });
    EXPECT(later);

    const auto result = dispatcher.dispatch(ValueEvent{});
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::IoFailure);
    EXPECT(
        result.error().message ==
        "EventDispatcher::dispatch handler failed: injected handler failure");
    EXPECT(later_handler_calls == 0);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_empty_function_handler_is_rejected() {
    Storm::EventDispatcher dispatcher;
    std::function<void(const ValueEvent&)> empty_handler;

    auto result = dispatcher.subscribe<ValueEvent>(std::move(empty_handler));
    EXPECT(!result);
    EXPECT(result.error().code == Storm::ErrorCode::InvalidArgument);

    const auto dispatched = dispatcher.dispatch(ValueEvent{});
    EXPECT(dispatched);
    EXPECT(dispatched.value() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_subscription_is_safe_after_dispatcher_destruction() {
    std::optional<Storm::EventDispatcher::Subscription> subscription;
    {
        Storm::EventDispatcher dispatcher;
        auto result = dispatcher.subscribe<ValueEvent>(
            [](const ValueEvent&) {});
        EXPECT(result);
        subscription.emplace(std::move(result).value());
        EXPECT(subscription->connected());
    }

    EXPECT(!subscription->connected());
    subscription->disconnect();
    EXPECT(!subscription->connected());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_explicit_disconnect_is_idempotent() {
    Storm::EventDispatcher dispatcher;
    int calls = 0;

    auto result = dispatcher.subscribe<ValueEvent>(
        [&calls](const ValueEvent&) { ++calls; });
    EXPECT(result);

    auto subscription = std::move(result).value();
    subscription.disconnect();
    subscription.disconnect();
    EXPECT(!subscription.connected());

    const auto dispatched = dispatcher.dispatch(ValueEvent{});
    EXPECT(dispatched);
    EXPECT(dispatched.value() == 0U);
    EXPECT(calls == 0);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"dispatch_order_and_exact_event_type", &test_dispatch_order_and_exact_event_type},
        TestCase{"dispatch_without_listeners_is_empty_success", &test_dispatch_without_listeners_is_empty_success},
        TestCase{"subscription_is_raii_and_move_only", &test_subscription_is_raii_and_move_only},
        TestCase{"disconnect_during_dispatch_skips_pending_handler", &test_disconnect_during_dispatch_skips_pending_handler},
        TestCase{"new_subscription_starts_on_next_dispatch", &test_new_subscription_starts_on_next_dispatch},
        TestCase{"clear_skips_pending_handlers", &test_clear_skips_pending_handlers},
        TestCase{"handler_failure_propagates_and_stops_dispatch", &test_handler_failure_propagates_and_stops_dispatch},
        TestCase{"empty_function_handler_is_rejected", &test_empty_function_handler_is_rejected},
        TestCase{"subscription_is_safe_after_dispatcher_destruction", &test_subscription_is_safe_after_dispatcher_destruction},
        TestCase{"explicit_disconnect_is_idempotent", &test_explicit_disconnect_is_idempotent},
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
