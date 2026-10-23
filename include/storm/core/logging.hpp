#pragma once

#include <cstdint>
#include <string_view>

namespace storm::core {

enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off
};

[[nodiscard]] constexpr bool is_valid_log_level(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
    case LogLevel::Debug:
    case LogLevel::Info:
    case LogLevel::Warning:
    case LogLevel::Error:
    case LogLevel::Critical:
    case LogLevel::Off:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr std::string_view log_level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
        return "Trace";
    case LogLevel::Debug:
        return "Debug";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    case LogLevel::Critical:
        return "Critical";
    case LogLevel::Off:
        return "Off";
    }
    return "Unknown";
}

// The record and its string views are valid only for the duration of the sink call.
struct LogRecord final {
    LogLevel level{LogLevel::Info};
    std::string_view category{};
    std::string_view message{};
};

// Sinks must not throw. The caller owns user_data and must keep it alive while used.
using LogSink = void (*)(void* user_data, const LogRecord& record) noexcept;

// Instance-based logging dispatch with no global state, formatting policy, or I/O dependency.
// This initial implementation is synchronous and not thread-safe; synchronize at the call site
// if multiple threads may write through the same Logger.
class Logger final {
public:
    explicit constexpr Logger(LogSink sink = nullptr, void* user_data = nullptr) noexcept
        : sink_(sink), user_data_(sink == nullptr ? nullptr : user_data) {}

    void set_sink(LogSink sink, void* user_data = nullptr) noexcept {
        sink_ = sink;
        user_data_ = sink == nullptr ? nullptr : user_data;
    }

    [[nodiscard]] bool set_minimum_level(LogLevel level) noexcept {
        if (!is_valid_log_level(level)) {
            return false;
        }
        minimum_level_ = level;
        return true;
    }

    [[nodiscard]] constexpr LogLevel minimum_level() const noexcept {
        return minimum_level_;
    }

    [[nodiscard]] bool is_enabled(LogLevel level) const noexcept {
        return sink_ != nullptr && is_valid_log_level(level) && level != LogLevel::Off &&
               minimum_level_ != LogLevel::Off &&
               static_cast<std::uint8_t>(level) >=
                   static_cast<std::uint8_t>(minimum_level_);
    }

    void write(LogLevel level, std::string_view category, std::string_view message) const noexcept {
        if (!is_enabled(level)) {
            return;
        }
        sink_(user_data_, LogRecord{level, category, message});
    }

private:
    LogSink sink_{};
    void* user_data_{};
    LogLevel minimum_level_{LogLevel::Info};
};

} // namespace storm::core
