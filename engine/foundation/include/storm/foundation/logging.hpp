#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace Storm {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

struct LogRecord final {
    LogLevel level{LogLevel::Info};
    std::string category;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) noexcept = 0;
};

// Logger owns its sink through shared ownership and has no platform dependency.
class Logger final {
public:
    explicit Logger(std::shared_ptr<ILogSink> sink) noexcept;

    void log(LogLevel level, std::string_view category, std::string_view message) const;

    [[nodiscard]] bool has_sink() const noexcept;

private:
    std::shared_ptr<ILogSink> sink_;
};

} // namespace Storm
