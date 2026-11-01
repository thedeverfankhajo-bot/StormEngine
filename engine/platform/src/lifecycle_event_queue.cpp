#include "storm/platform/lifecycle_event_queue.hpp"

namespace Storm::Platform {

LifecycleQueuePushResult LifecycleEventQueue::try_push(LifecycleState state) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return LifecycleQueuePushResult::Closed;
    }
    if (size_ == capacity) {
        return LifecycleQueuePushResult::Full;
    }

    const std::size_t tail = (head_ + size_) % capacity;
    events_[tail] = state;
    ++size_;
    return LifecycleQueuePushResult::Enqueued;
}

std::optional<LifecycleState> LifecycleEventQueue::try_pop() {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == 0U) {
        return std::nullopt;
    }

    const LifecycleState event = events_[head_];
    head_ = (head_ + 1U) % capacity;
    --size_;
    return event;
}

void LifecycleEventQueue::close() {
    const std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
}

bool LifecycleEventQueue::is_closed() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

std::size_t LifecycleEventQueue::size() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

} // namespace Storm::Platform
