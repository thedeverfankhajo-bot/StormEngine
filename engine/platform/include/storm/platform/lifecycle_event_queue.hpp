#pragma once

#include "storm/platform/lifecycle.hpp"

#include <array>
#include <cstddef>
#include <mutex>
#include <optional>

namespace Storm::Platform {

enum class LifecycleQueuePushResult {
    Enqueued,
    Full,
    Closed,
};

// A bounded, allocation-free queue for forwarding native lifecycle callbacks
// to the engine thread. Multiple producers are serialized; one consumer is
// expected to apply events to LifecycleStateMachine in FIFO order.
class LifecycleEventQueue final {
public:
    static constexpr std::size_t capacity = 64U;

    LifecycleEventQueue() = default;
    LifecycleEventQueue(const LifecycleEventQueue&) = delete;
    LifecycleEventQueue& operator=(const LifecycleEventQueue&) = delete;
    LifecycleEventQueue(LifecycleEventQueue&&) = delete;
    LifecycleEventQueue& operator=(LifecycleEventQueue&&) = delete;

    [[nodiscard]] LifecycleQueuePushResult try_push(LifecycleState state);
    [[nodiscard]] std::optional<LifecycleState> try_pop();

    // Closing rejects future producers but leaves queued events available to drain.
    void close();

    [[nodiscard]] bool is_closed() const;
    [[nodiscard]] std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::array<LifecycleState, capacity> events_{};
    std::size_t head_{0U};
    std::size_t size_{0U};
    bool closed_{false};
};

} // namespace Storm::Platform
