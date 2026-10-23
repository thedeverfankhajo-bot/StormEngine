#pragma once

#include "storm/core/status.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace storm::core {

enum class RuntimeState : std::uint8_t {
    Uninitialized,
    Initialized,
    Running,
    Paused,
    Stopping,
    Stopped,
    Failed
};

[[nodiscard]] constexpr std::string_view runtime_state_name(RuntimeState state) noexcept {
    switch (state) {
    case RuntimeState::Uninitialized:
        return "Uninitialized";
    case RuntimeState::Initialized:
        return "Initialized";
    case RuntimeState::Running:
        return "Running";
    case RuntimeState::Paused:
        return "Paused";
    case RuntimeState::Stopping:
        return "Stopping";
    case RuntimeState::Stopped:
        return "Stopped";
    case RuntimeState::Failed:
        return "Failed";
    }
    return "Unknown";
}

// A single-owner-thread lifecycle state machine for the runtime shell.
// Platform/resource initialization and teardown are performed by the caller;
// this class validates state transitions and preserves a fatal failure reason.
class RuntimeLifecycle final {
public:
    [[nodiscard]] RuntimeState state() const noexcept {
        return state_;
    }

    [[nodiscard]] bool is_running() const noexcept {
        return state_ == RuntimeState::Running;
    }

    [[nodiscard]] const std::optional<Status>& failure_reason() const noexcept {
        return failure_reason_;
    }

    [[nodiscard]] Status initialize() {
        return transition(RuntimeState::Uninitialized, RuntimeState::Initialized, "initialize");
    }

    [[nodiscard]] Status start() {
        return transition(RuntimeState::Initialized, RuntimeState::Running, "start");
    }

    [[nodiscard]] Status pause() {
        return transition(RuntimeState::Running, RuntimeState::Paused, "pause");
    }

    [[nodiscard]] Status resume() {
        return transition(RuntimeState::Paused, RuntimeState::Running, "resume");
    }

    [[nodiscard]] Status request_stop() {
        switch (state_) {
        case RuntimeState::Initialized:
        case RuntimeState::Running:
        case RuntimeState::Paused:
            state_ = RuntimeState::Stopping;
            return Status::success();
        case RuntimeState::Uninitialized:
        case RuntimeState::Stopping:
        case RuntimeState::Stopped:
        case RuntimeState::Failed:
            return invalid_transition("request_stop");
        }
        return invalid_transition("request_stop");
    }

    // Call finish_stop only after the owner has released runtime resources.
    [[nodiscard]] Status finish_stop() {
        return transition(RuntimeState::Stopping, RuntimeState::Stopped, "finish_stop");
    }

    // Records the fatal error, moves the runtime to Failed, and returns that error
    // so callers can propagate or display it. Failed and Stopped are terminal until reset.
    [[nodiscard]] Status fail(ErrorCode code, std::string message) {
        if (state_ == RuntimeState::Stopped || state_ == RuntimeState::Failed) {
            return invalid_transition("fail");
        }

        failure_reason_.emplace(Status::failure(code, std::move(message)));
        state_ = RuntimeState::Failed;
        return *failure_reason_;
    }

    // Restart is explicit. Resetting a live or partially initialized runtime is rejected.
    [[nodiscard]] Status reset() {
        if (state_ != RuntimeState::Stopped && state_ != RuntimeState::Failed) {
            return invalid_transition("reset");
        }

        failure_reason_.reset();
        state_ = RuntimeState::Uninitialized;
        return Status::success();
    }

private:
    [[nodiscard]] Status transition(RuntimeState expected, RuntimeState next,
                                   std::string_view operation) {
        if (state_ != expected) {
            return invalid_transition(operation);
        }

        state_ = next;
        return Status::success();
    }

    [[nodiscard]] Status invalid_transition(std::string_view operation) const {
        std::string message;
        message.reserve(operation.size() + 48U);
        message.append(operation.data(), operation.size());
        message.append(" is not valid while runtime is ");
        const auto current_name = runtime_state_name(state_);
        message.append(current_name.data(), current_name.size());
        return Status::failure(ErrorCode::InvalidState, std::move(message));
    }

    RuntimeState state_{RuntimeState::Uninitialized};
    std::optional<Status> failure_reason_{};
};

} // namespace storm::core
