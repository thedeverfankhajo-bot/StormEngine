#include "storm/core/engine.hpp"
#include "storm/foundation/logging.hpp"
#include "storm/foundation/memory_budget.hpp"
#include "storm/foundation/result.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <iterator>
#include <thread>
#include <string>
#include <utility>
#include <vector>

namespace {

struct TestFailure final {
    std::string message;
};

#define STORM_EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return TestFailure{std::string("Expectation failed: ") + #condition}; \
        } \
    } while (false)

class RecordingSink final : public Storm::ILogSink {
public:
    void write(const Storm::LogRecord& record) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            records_.push_back(record);
        } catch (...) {
            // A test sink cannot propagate exceptions through the noexcept logging contract.
            failed_.store(true, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] std::vector<Storm::LogRecord> records() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_;
    }

    [[nodiscard]] bool failed() const noexcept {
        return failed_.load(std::memory_order_relaxed);
    }

private:
    mutable std::mutex mutex_;
    std::vector<Storm::LogRecord> records_;
    std::atomic<bool> failed_{false};
};

[[nodiscard]] std::optional<TestFailure> test_result_type() {
    auto success = Storm::Result<int>::success(42);
    STORM_EXPECT(success.has_value());
    STORM_EXPECT(success.value() == 42);

    auto failure = Storm::Result<int>::failure(
        Storm::Error::make(Storm::ErrorCode::InvalidArgument, "invalid input"));
    STORM_EXPECT(!failure.has_value());
    STORM_EXPECT(failure.error().code == Storm::ErrorCode::InvalidArgument);
    STORM_EXPECT(failure.error().message == "invalid input");

    auto void_success = Storm::Result<void>::success();
    STORM_EXPECT(void_success.has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<TestFailure> test_memory_budget() {
    Storm::MemoryBudget budget(1024U);
    STORM_EXPECT(budget.try_reserve(512U));
    STORM_EXPECT(budget.reserved_bytes() == 512U);
    STORM_EXPECT(budget.available_bytes() == 512U);
    STORM_EXPECT(!budget.try_reserve(513U));
    STORM_EXPECT(!budget.try_reserve(1025U));
    STORM_EXPECT(!budget.release(513U));
    STORM_EXPECT(budget.release(128U));
    STORM_EXPECT(budget.reserved_bytes() == 384U);
    STORM_EXPECT(budget.try_reserve(640U));
    STORM_EXPECT(budget.reserved_bytes() == 1024U);
    STORM_EXPECT(!budget.try_reserve(1U));
    return std::nullopt;
}

[[nodiscard]] std::optional<TestFailure> test_memory_budget_concurrency() {
    Storm::MemoryBudget budget(64U);
    std::atomic<bool> start{false};
    std::atomic<bool> exceeded_capacity{false};
    std::vector<std::thread> workers;
    workers.reserve(8U);

    for (std::size_t worker = 0U; worker < 8U; ++worker) {
        workers.emplace_back([&budget, &start, &exceeded_capacity]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (std::size_t iteration = 0U; iteration < 5000U; ++iteration) {
                if (budget.try_reserve(1U)) {
                    if (budget.reserved_bytes() > budget.capacity_bytes()) {
                        exceeded_capacity.store(true, std::memory_order_relaxed);
                    }
                    const bool released = budget.release(1U);
                    if (!released) {
                        exceeded_capacity.store(true, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    start.store(true, std::memory_order_release);
    for (std::thread& worker : workers) {
        worker.join();
    }

    STORM_EXPECT(!exceeded_capacity.load(std::memory_order_relaxed));
    STORM_EXPECT(budget.reserved_bytes() == 0U);
    return std::nullopt;
}

[[nodiscard]] std::optional<TestFailure> test_logger() {
    auto sink = std::make_shared<RecordingSink>();
    const Storm::Logger logger(sink);
    STORM_EXPECT(logger.has_sink());

    logger.log(Storm::LogLevel::Warning, "foundation", "budget is nearly full");
    const auto records = sink->records();
    STORM_EXPECT(!sink->failed());
    STORM_EXPECT(records.size() == 1U);
    STORM_EXPECT(records.front().level == Storm::LogLevel::Warning);
    STORM_EXPECT(records.front().category == "foundation");
    STORM_EXPECT(records.front().message == "budget is nearly full");
    STORM_EXPECT(records.front().thread_id == std::this_thread::get_id());

    const Storm::Logger silent_logger(nullptr);
    STORM_EXPECT(!silent_logger.has_sink());
    silent_logger.log(Storm::LogLevel::Info, "test", "this is intentionally discarded");
    return std::nullopt;
}

[[nodiscard]] std::optional<TestFailure> test_engine_lifecycle() {
    Storm::Engine engine;
    STORM_EXPECT(engine.state() == Storm::EngineState::Created);

    auto invalid = engine.initialize(Storm::EngineConfig{.application_name = " \t\n"});
    STORM_EXPECT(!invalid.has_value());
    STORM_EXPECT(invalid.error().code == Storm::ErrorCode::InvalidArgument);
    STORM_EXPECT(engine.state() == Storm::EngineState::Created);

    auto initialized = engine.initialize(Storm::EngineConfig{.application_name = "Storm Sample"});
    STORM_EXPECT(initialized.has_value());
    STORM_EXPECT(engine.state() == Storm::EngineState::Initialized);
    STORM_EXPECT(engine.config().application_name == "Storm Sample");

    auto repeated = engine.initialize(Storm::EngineConfig{.application_name = "Another"});
    STORM_EXPECT(!repeated.has_value());
    STORM_EXPECT(repeated.error().code == Storm::ErrorCode::InvalidState);

    STORM_EXPECT(engine.shutdown().has_value());
    STORM_EXPECT(engine.state() == Storm::EngineState::Shutdown);
    STORM_EXPECT(engine.shutdown().has_value());
    STORM_EXPECT(!engine.initialize(Storm::EngineConfig{.application_name = "Restart"}).has_value());
    return std::nullopt;
}

using TestFunction = std::optional<TestFailure> (*)();

struct TestCase final {
    const char* name;
    TestFunction function;
};

} // namespace

int main() {
    constexpr TestCase tests[]{
        {"result_type", &test_result_type},
        {"memory_budget", &test_memory_budget},
        {"memory_budget_concurrency", &test_memory_budget_concurrency},
        {"logger", &test_logger},
        {"engine_lifecycle", &test_engine_lifecycle},
    };

    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto failure = test.function();
        if (failure.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << failure->message << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }

    std::cout << "Executed " << std::size(tests) << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
