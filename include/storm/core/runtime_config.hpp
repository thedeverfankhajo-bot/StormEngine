#pragma once

#include "storm/core/fixed_step_clock.hpp"
#include "storm/core/logging.hpp"
#include "storm/core/status.hpp"

namespace storm::core {

// In-memory, validated configuration for the runtime kernel.
// File loading and migration belong to a future project-configuration layer.
struct RuntimeConfig final {
    FixedStepClockConfig simulation{};
    LogLevel minimum_log_level{LogLevel::Info};

    [[nodiscard]] Status validate() const {
        if (!FixedStepClock::is_valid_config(simulation)) {
            return Status::failure(
                ErrorCode::InvalidArgument, "runtime simulation clock configuration is invalid");
        }

        if (!is_valid_log_level(minimum_log_level)) {
            return Status::failure(
                ErrorCode::InvalidArgument, "runtime minimum log level is invalid");
        }

        return Status::success();
    }
};

} // namespace storm::core
