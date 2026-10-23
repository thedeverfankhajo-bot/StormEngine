#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace storm::core {

struct FixedStepClockConfig final {
    double fixed_delta_seconds{1.0 / 60.0};
    double max_frame_delta_seconds{0.25};
    std::uint32_t max_steps_per_frame{8};
};

struct FixedStepAdvanceResult final {
    // False only when the supplied frame delta was invalid; clock state is unchanged.
    bool accepted{};
    std::uint32_t steps{};
    double simulated_seconds{};
    // Time intentionally not simulated: frame-delta clamp plus whole-step backlog discarded.
    double discarded_seconds{};
    // Fraction of a fixed step left for render interpolation; always in [0, 1).
    double interpolation_alpha{};
};

// Accumulates caller-supplied frame deltas and emits a bounded number of fixed simulation steps.
// This type does not read the system clock and can therefore be tested deterministically.
class FixedStepClock final {
public:
    [[nodiscard]] bool configure(FixedStepClockConfig config) noexcept {
        if (!is_valid_config(config)) {
            return false;
        }

        config_ = config;
        accumulator_seconds_ = 0.0;
        return true;
    }

    [[nodiscard]] const FixedStepClockConfig& config() const noexcept {
        return config_;
    }

    [[nodiscard]] double accumulator_seconds() const noexcept {
        return accumulator_seconds_;
    }

    void reset() noexcept {
        accumulator_seconds_ = 0.0;
    }

    [[nodiscard]] FixedStepAdvanceResult advance(double frame_delta_seconds) noexcept {
        if (!std::isfinite(frame_delta_seconds) || frame_delta_seconds < 0.0) {
            return {false, 0U, 0.0, 0.0, interpolation_alpha()};
        }

        const double accepted_delta =
            std::min(frame_delta_seconds, config_.max_frame_delta_seconds);
        double discarded = frame_delta_seconds - accepted_delta;
        accumulator_seconds_ += accepted_delta;

        std::uint32_t steps = 0U;
        while (steps < config_.max_steps_per_frame) {
            const double tolerance =
                std::numeric_limits<double>::epsilon() *
                std::max(accumulator_seconds_, config_.fixed_delta_seconds) * 8.0;
            if (accumulator_seconds_ + tolerance < config_.fixed_delta_seconds) {
                break;
            }

            accumulator_seconds_ =
                std::max(0.0, accumulator_seconds_ - config_.fixed_delta_seconds);
            ++steps;
        }

        // Avoid the "spiral of death": if the step budget was exhausted, drop any
        // remaining complete ticks but preserve the fractional remainder for interpolation.
        if (steps == config_.max_steps_per_frame) {
            const double fixed_delta = config_.fixed_delta_seconds;
            const double quotient = accumulator_seconds_ / fixed_delta;
            const double quotient_tolerance =
                std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(quotient)) *
                8.0;
            const double complete_backlog_steps = std::floor(quotient + quotient_tolerance);

            if (complete_backlog_steps >= 1.0) {
                const double before_drop = accumulator_seconds_;
                const double remainder =
                    accumulator_seconds_ - (complete_backlog_steps * fixed_delta);
                accumulator_seconds_ = std::max(0.0, remainder);
                discarded += before_drop - accumulator_seconds_;
            }
        }

        return {true, steps, static_cast<double>(steps) * config_.fixed_delta_seconds,
                discarded, interpolation_alpha()};
    }

private:
    [[nodiscard]] static bool is_valid_config(const FixedStepClockConfig& config) noexcept {
        constexpr double minimum_fixed_delta = 1.0e-9;
        constexpr double maximum_fixed_delta = 1.0;
        constexpr double maximum_frame_delta = 60.0;
        constexpr std::uint32_t maximum_steps_per_frame = 1024U;

        return std::isfinite(config.fixed_delta_seconds) &&
               std::isfinite(config.max_frame_delta_seconds) &&
               config.fixed_delta_seconds >= minimum_fixed_delta &&
               config.fixed_delta_seconds <= maximum_fixed_delta &&
               config.max_frame_delta_seconds >= config.fixed_delta_seconds &&
               config.max_frame_delta_seconds <= maximum_frame_delta &&
               config.max_steps_per_frame >= 1U &&
               config.max_steps_per_frame <= maximum_steps_per_frame;
    }

    [[nodiscard]] double interpolation_alpha() const noexcept {
        const double alpha = accumulator_seconds_ / config_.fixed_delta_seconds;
        return std::clamp(alpha, 0.0, std::nextafter(1.0, 0.0));
    }

    FixedStepClockConfig config_{};
    double accumulator_seconds_{};
};

} // namespace storm::core
