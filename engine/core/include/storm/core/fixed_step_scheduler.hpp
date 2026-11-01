#pragma once

#include "storm/foundation/result.hpp"

#include <chrono>
#include <cstdint>

namespace Storm {

struct FixedStepConfig final {
    std::chrono::nanoseconds fixed_step{16'666'667};
    std::chrono::nanoseconds maximum_frame_delta{250'000'000};
    std::uint32_t maximum_steps_per_frame{8U};
};

struct FixedStepAdvance final {
    std::uint32_t step_count{0U};
    double interpolation_alpha{0.0};
    std::chrono::nanoseconds accepted_frame_delta{0};
    std::chrono::nanoseconds clamped_frame_delta{0};
    std::chrono::nanoseconds discarded_backlog{0};
};

// Accumulates elapsed time and reports a bounded number of fixed updates.
// It does not run callbacks, own a clock, or implement a game loop.
class FixedStepScheduler final {
public:
    [[nodiscard]] static Result<FixedStepScheduler> create(FixedStepConfig config);

    [[nodiscard]] Result<FixedStepAdvance> advance(
        std::chrono::nanoseconds frame_delta);

    void reset() noexcept;

    [[nodiscard]] const FixedStepConfig& config() const noexcept;
    [[nodiscard]] std::chrono::nanoseconds remaining_accumulator() const noexcept;

private:
    explicit FixedStepScheduler(FixedStepConfig config) noexcept;

    FixedStepConfig config_;
    std::chrono::nanoseconds accumulator_{0};
};

} // namespace Storm
