#pragma once

#include <atomic>
#include <cstddef>

namespace Storm {

// Tracks a caller-defined byte budget. It does not allocate or free memory.
class MemoryBudget final {
public:
    explicit MemoryBudget(std::size_t capacity_bytes) noexcept
        : capacity_bytes_(capacity_bytes) {}

    MemoryBudget(const MemoryBudget&) = delete;
    MemoryBudget& operator=(const MemoryBudget&) = delete;

    [[nodiscard]] bool try_reserve(std::size_t bytes) noexcept {
        if (bytes > capacity_bytes_) {
            return false;
        }

        std::size_t current = reserved_bytes_.load(std::memory_order_relaxed);
        while (current <= capacity_bytes_ - bytes) {
            if (reserved_bytes_.compare_exchange_weak(
                    current,
                    current + bytes,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool release(std::size_t bytes) noexcept {
        std::size_t current = reserved_bytes_.load(std::memory_order_relaxed);
        while (current >= bytes) {
            if (reserved_bytes_.compare_exchange_weak(
                    current,
                    current - bytes,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::size_t capacity_bytes() const noexcept {
        return capacity_bytes_;
    }

    [[nodiscard]] std::size_t reserved_bytes() const noexcept {
        return reserved_bytes_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t available_bytes() const noexcept {
        const std::size_t reserved = reserved_bytes();
        return reserved <= capacity_bytes_ ? capacity_bytes_ - reserved : 0U;
    }

private:
    const std::size_t capacity_bytes_;
    std::atomic<std::size_t> reserved_bytes_{0U};
};

} // namespace Storm
