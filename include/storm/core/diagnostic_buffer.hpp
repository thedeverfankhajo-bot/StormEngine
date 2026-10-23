#pragma once

#include "storm/core/logging.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace storm::core {

struct DiagnosticEntryView final {
    LogLevel level{LogLevel::Info};
    std::string_view category{};
    std::string_view message{};
    bool category_truncated{};
    bool message_truncated{};
};

// A fixed-capacity, no-heap diagnostic ring. Capacity and maximum text bytes are part of the
// type, so memory use is known at compile time. Oldest entries are overwritten when full.
template <std::size_t Capacity, std::size_t MaxCategoryBytes = 48U,
          std::size_t MaxMessageBytes = 192U>
class DiagnosticBuffer final {
    static_assert(Capacity > 0U, "DiagnosticBuffer capacity must be greater than zero");
    static_assert(MaxCategoryBytes > 0U,
                  "DiagnosticBuffer category capacity must be greater than zero");
    static_assert(MaxMessageBytes > 0U,
                  "DiagnosticBuffer message capacity must be greater than zero");

public:
    static constexpr std::size_t capacity = Capacity;
    static constexpr std::size_t max_category_bytes = MaxCategoryBytes;
    static constexpr std::size_t max_message_bytes = MaxMessageBytes;

    // Returns false for invalid severities or Off. Accepted records are copied synchronously.
    [[nodiscard]] bool append(const LogRecord& record) noexcept {
        if (!is_valid_log_level(record.level) || record.level == LogLevel::Off) {
            return false;
        }

        std::size_t destination = 0U;
        if (size_ < Capacity) {
            destination = (oldest_index_ + size_) % Capacity;
            ++size_;
        } else {
            destination = oldest_index_;
            oldest_index_ = (oldest_index_ + 1U) % Capacity;
            saturating_increment(dropped_count_);
        }

        StoredEntry& entry = entries_[destination];
        entry.level = record.level;
        entry.category_length = copy_text(entry.category, record.category);
        entry.message_length = copy_text(entry.message, record.message);
        entry.category_truncated = record.category.size() > MaxCategoryBytes;
        entry.message_truncated = record.message.size() > MaxMessageBytes;

        saturating_increment(total_record_count_);
        saturating_increment(level_counts_[static_cast<std::size_t>(record.level)]);
        return true;
    }

    // Compatible with Logger's synchronous LogSink callback signature.
    static void capture_sink(void* user_data, const LogRecord& record) noexcept {
        if (user_data != nullptr) {
            (void)static_cast<DiagnosticBuffer*>(user_data)->append(record);
        }
    }

    // Records are enumerated oldest-first. Returned views remain valid only until that slot is
    // overwritten, clear() is called, or this buffer is destroyed.
    [[nodiscard]] std::optional<DiagnosticEntryView> at(std::size_t oldest_index) const noexcept {
        if (oldest_index >= size_) {
            return std::nullopt;
        }

        const std::size_t slot = (oldest_index_ + oldest_index) % Capacity;
        const StoredEntry& entry = entries_[slot];
        return DiagnosticEntryView{
            entry.level,
            std::string_view{entry.category.data(), entry.category_length},
            std::string_view{entry.message.data(), entry.message_length},
            entry.category_truncated,
            entry.message_truncated
        };
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept {
        return Capacity;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0U;
    }

    // Cumulative counters since construction/clear; they include entries later evicted.
    [[nodiscard]] std::uint64_t total_record_count() const noexcept {
        return total_record_count_;
    }

    [[nodiscard]] std::uint64_t dropped_count() const noexcept {
        return dropped_count_;
    }

    [[nodiscard]] std::uint64_t count_for_level(LogLevel level) const noexcept {
        if (!is_valid_log_level(level) || level == LogLevel::Off) {
            return 0U;
        }
        return level_counts_[static_cast<std::size_t>(level)];
    }

    void clear() noexcept {
        for (StoredEntry& entry : entries_) {
            entry = StoredEntry{};
        }
        level_counts_.fill(0U);
        oldest_index_ = 0U;
        size_ = 0U;
        total_record_count_ = 0U;
        dropped_count_ = 0U;
    }

private:
    struct StoredEntry final {
        LogLevel level{LogLevel::Info};
        std::array<char, MaxCategoryBytes + 1U> category{};
        std::array<char, MaxMessageBytes + 1U> message{};
        std::size_t category_length{};
        std::size_t message_length{};
        bool category_truncated{};
        bool message_truncated{};
    };

    template <std::size_t BufferSize>
    [[nodiscard]] static std::size_t copy_text(std::array<char, BufferSize>& destination,
                                                std::string_view source) noexcept {
        constexpr std::size_t writable_bytes = BufferSize - 1U;
        const std::size_t length =
            source.size() < writable_bytes ? source.size() : writable_bytes;
        for (std::size_t index = 0U; index < length; ++index) {
            destination[index] = source[index];
        }
        destination[length] = '\0';
        return length;
    }

    static void saturating_increment(std::uint64_t& value) noexcept {
        if (value < std::numeric_limits<std::uint64_t>::max()) {
            ++value;
        }
    }

    std::array<StoredEntry, Capacity> entries_{};
    std::array<std::uint64_t, static_cast<std::size_t>(LogLevel::Off) + 1U> level_counts_{};
    std::size_t oldest_index_{};
    std::size_t size_{};
    std::uint64_t total_record_count_{};
    std::uint64_t dropped_count_{};
};

} // namespace storm::core
