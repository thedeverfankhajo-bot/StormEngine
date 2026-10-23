#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace storm::core {

enum class ErrorCode : std::uint16_t {
    None = 0,
    InvalidArgument,
    InvalidState,
    Unsupported,
    NotFound,
    Io,
    Parse,
    ResourceExhausted,
    Internal,
    AlreadyExists
};

class Status final {
public:
    [[nodiscard]] static Status success() noexcept {
        return Status{};
    }

    [[nodiscard]] static Status failure(ErrorCode code, std::string message) {
        // A failed Status must never look successful because of a caller mistake.
        if (code == ErrorCode::None) {
            code = ErrorCode::Internal;
        }
        return Status{code, std::move(message)};
    }

    [[nodiscard]] bool ok() const noexcept {
        return code_ == ErrorCode::None;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return ok();
    }

    [[nodiscard]] ErrorCode code() const noexcept {
        return code_;
    }

    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

private:
    Status() = default;

    Status(ErrorCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    ErrorCode code_{ErrorCode::None};
    std::string message_{};
};

} // namespace storm::core
