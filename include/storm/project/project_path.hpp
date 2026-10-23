#pragma once

#include "storm/core/status.hpp"

#include <cstddef>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace storm::project {

// A portable logical path relative to a project root. This validates path syntax only; it does
// not access the filesystem, resolve symlinks, or provide a filesystem sandbox.
class ProjectPath final {
public:
    static constexpr std::size_t maximum_path_bytes = 1024U;
    static constexpr std::size_t maximum_segment_bytes = 255U;
    static constexpr std::size_t maximum_segments = 64U;

    ProjectPath() noexcept = default;

    // Parses a canonical, slash-delimited, project-relative UTF-8 path. On failure, output is
    // unchanged. Case and Unicode normalization are intentionally preserved byte-for-byte.
    [[nodiscard]] static core::Status parse(std::string_view input, ProjectPath& output) {
        if (input.empty() || input.front() == '/' || input.back() == '/') {
            return invalid_path("project path must be non-empty, relative, and have no trailing slash");
        }
        if (input.size() > maximum_path_bytes) {
            return core::Status::failure(
                core::ErrorCode::ResourceExhausted, "project path exceeds the byte limit");
        }
        if (!is_valid_utf8(input)) {
            return invalid_path("project path is not valid UTF-8");
        }

        for (const unsigned char character : input) {
            if (character < 0x20U || character == 0x7fU || is_forbidden_ascii(character)) {
                return invalid_path("project path contains a forbidden character");
            }
        }

        std::size_t segment_count = 0U;
        std::size_t start = 0U;
        while (start < input.size()) {
            const std::size_t slash = input.find('/', start);
            const std::size_t end = slash == std::string_view::npos ? input.size() : slash;
            const std::string_view segment = input.substr(start, end - start);

            if (segment.empty() || segment == "." || segment == "..") {
                return invalid_path("project path contains an empty or traversal segment");
            }
            if (segment.size() > maximum_segment_bytes) {
                return core::Status::failure(
                    core::ErrorCode::ResourceExhausted, "project path segment exceeds the byte limit");
            }
            if (segment.back() == '.' || segment.back() == ' ') {
                return invalid_path("project path segments cannot end with a dot or space");
            }
            if (is_reserved_windows_device_name(segment)) {
                return invalid_path("project path contains a reserved Windows device name");
            }

            ++segment_count;
            if (segment_count > maximum_segments) {
                return core::Status::failure(
                    core::ErrorCode::ResourceExhausted, "project path contains too many segments");
            }

            if (slash == std::string_view::npos) {
                break;
            }
            start = slash + 1U;
        }

        try {
            std::string candidate{input};
            value_.swap(candidate);
        } catch (const std::bad_alloc&) {
            return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
        }

        return core::Status::success();
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return !value_.empty();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return is_valid();
    }

    [[nodiscard]] std::string_view value() const noexcept {
        return value_;
    }

    [[nodiscard]] friend bool operator==(const ProjectPath&, const ProjectPath&) noexcept = default;

private:
    [[nodiscard]] static core::Status invalid_path(std::string message) {
        return core::Status::failure(core::ErrorCode::InvalidArgument, std::move(message));
    }

    [[nodiscard]] static constexpr bool is_forbidden_ascii(unsigned char value) noexcept {
        switch (value) {
        case '\\':
        case ':':
        case '<':
        case '>':
        case '"':
        case '|':
        case '?':
        case '*':
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] static constexpr unsigned char ascii_upper(unsigned char value) noexcept {
        return value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')
                   ? static_cast<unsigned char>(value - static_cast<unsigned char>('a') +
                                                static_cast<unsigned char>('A'))
                   : value;
    }

    [[nodiscard]] static constexpr bool ascii_equal_ignore_case(std::string_view left,
                                                                 std::string_view right) noexcept {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t index = 0U; index < left.size(); ++index) {
            if (ascii_upper(static_cast<unsigned char>(left[index])) !=
                ascii_upper(static_cast<unsigned char>(right[index]))) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr bool is_reserved_windows_device_name(
        std::string_view segment) noexcept {
        const std::size_t dot = segment.find('.');
        const std::string_view base = segment.substr(0U, dot);

        if (ascii_equal_ignore_case(base, "CON") ||
            ascii_equal_ignore_case(base, "PRN") ||
            ascii_equal_ignore_case(base, "AUX") ||
            ascii_equal_ignore_case(base, "NUL") ||
            ascii_equal_ignore_case(base, "CONIN$") ||
            ascii_equal_ignore_case(base, "CONOUT$")) {
            return true;
        }

        const bool serial_prefix =
            base.size() >= 3U &&
            (ascii_equal_ignore_case(base.substr(0U, 3U), "COM") ||
             ascii_equal_ignore_case(base.substr(0U, 3U), "LPT"));
        if (!serial_prefix) {
            return false;
        }

        if (base.size() == 4U && base[3] >= '1' && base[3] <= '9') {
            return true;
        }

        // Windows also reserves COM/LPT names with superscript 1, 2 or 3.
        return base.size() == 5U &&
               static_cast<unsigned char>(base[3]) == 0xc2U &&
               (static_cast<unsigned char>(base[4]) == 0xb9U ||
                static_cast<unsigned char>(base[4]) == 0xb2U ||
                static_cast<unsigned char>(base[4]) == 0xb3U);
    }

    [[nodiscard]] static constexpr bool is_continuation(unsigned char value) noexcept {
        return value >= 0x80U && value <= 0xbfU;
    }

    [[nodiscard]] static bool is_valid_utf8(std::string_view input) noexcept {
        std::size_t index = 0U;
        while (index < input.size()) {
            const auto first = static_cast<unsigned char>(input[index]);
            if (first <= 0x7fU) {
                ++index;
                continue;
            }

            if (first >= 0xc2U && first <= 0xdfU) {
                if (index + 1U >= input.size() ||
                    !is_continuation(static_cast<unsigned char>(input[index + 1U]))) {
                    return false;
                }
                index += 2U;
                continue;
            }

            if (first >= 0xe0U && first <= 0xefU) {
                if (index + 2U >= input.size()) {
                    return false;
                }
                const auto second = static_cast<unsigned char>(input[index + 1U]);
                const auto third = static_cast<unsigned char>(input[index + 2U]);
                if (!is_continuation(third) ||
                    (first == 0xe0U && (second < 0xa0U || second > 0xbfU)) ||
                    (first == 0xedU && (second < 0x80U || second > 0x9fU)) ||
                    ((first != 0xe0U && first != 0xedU) && !is_continuation(second))) {
                    return false;
                }
                index += 3U;
                continue;
            }

            if (first >= 0xf0U && first <= 0xf4U) {
                if (index + 3U >= input.size()) {
                    return false;
                }
                const auto second = static_cast<unsigned char>(input[index + 1U]);
                const auto third = static_cast<unsigned char>(input[index + 2U]);
                const auto fourth = static_cast<unsigned char>(input[index + 3U]);
                if (!is_continuation(third) || !is_continuation(fourth) ||
                    (first == 0xf0U && (second < 0x90U || second > 0xbfU)) ||
                    (first == 0xf4U && (second < 0x80U || second > 0x8fU)) ||
                    (first != 0xf0U && first != 0xf4U && !is_continuation(second))) {
                    return false;
                }
                index += 4U;
                continue;
            }

            return false;
        }
        return true;
    }

    std::string value_{};
};

} // namespace storm::project
