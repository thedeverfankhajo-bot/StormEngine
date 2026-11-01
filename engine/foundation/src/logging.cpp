#include "storm/foundation/logging.hpp"

#include <utility>

namespace Storm {

Logger::Logger(std::shared_ptr<ILogSink> sink) noexcept
    : sink_(std::move(sink)) {}

void Logger::log(
    LogLevel level,
    std::string_view category,
    std::string_view message) const {
    if (!sink_) {
        return;
    }

    const LogRecord record{
        .level = level,
        .category = std::string(category),
        .message = std::string(message),
        .timestamp = std::chrono::system_clock::now(),
        .thread_id = std::this_thread::get_id(),
    };
    sink_->write(record);
}

bool Logger::has_sink() const noexcept {
    return static_cast<bool>(sink_);
}

} // namespace Storm
