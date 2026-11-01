#include "storm/core/engine.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace Storm {
namespace {

[[nodiscard]] bool contains_non_whitespace(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isspace(character) == 0;
    });
}

} // namespace

Result<void> Engine::initialize(EngineConfig config) {
    if (state_ != EngineState::Created) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "Engine can only be initialized from the Created state"));
    }

    if (!contains_non_whitespace(config.application_name)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "application_name must contain at least one non-whitespace character"));
    }

    config_ = std::move(config);
    state_ = EngineState::Initialized;
    return Result<void>::success();
}

Result<void> Engine::shutdown() {
    if (state_ == EngineState::Shutdown) {
        return Result<void>::success();
    }

    state_ = EngineState::Shutdown;
    return Result<void>::success();
}

EngineState Engine::state() const noexcept {
    return state_;
}

const EngineConfig& Engine::config() const noexcept {
    return config_;
}

} // namespace Storm
