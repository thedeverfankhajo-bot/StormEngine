#pragma once

#include <string>
#include <utility>

namespace Storm {

enum class ErrorCode {
    InvalidArgument,
    InvalidState,
    ResourceExhausted,
    IoFailure,
    Unsupported,
    Internal,
};

struct Error final {
    ErrorCode code{ErrorCode::Internal};
    std::string message;

    [[nodiscard]] static Error make(ErrorCode error_code, std::string error_message) {
        return Error{error_code, std::move(error_message)};
    }
};

} // namespace Storm
