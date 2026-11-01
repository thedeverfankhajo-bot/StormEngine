#pragma once

#include "storm/foundation/error.hpp"

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace Storm {

template <typename T>
class [[nodiscard]] Result final {
    static_assert(!std::is_reference_v<T>, "Result<T> cannot hold a reference");
    static_assert(!std::is_void_v<T>, "Use Result<void> for operations without a value");

public:
    [[nodiscard]] static Result success(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }

    [[nodiscard]] static Result failure(Error error) {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] bool has_value() const noexcept {
        return storage_.index() == 0U;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] T& value() & {
        if (!has_value()) {
            throw std::logic_error("Attempted to access the value of a failed Result");
        }
        return std::get<0>(storage_);
    }

    [[nodiscard]] const T& value() const& {
        if (!has_value()) {
            throw std::logic_error("Attempted to access the value of a failed Result");
        }
        return std::get<0>(storage_);
    }

    [[nodiscard]] T&& value() && {
        if (!has_value()) {
            throw std::logic_error("Attempted to access the value of a failed Result");
        }
        return std::get<0>(std::move(storage_));
    }

    [[nodiscard]] Error& error() & {
        if (has_value()) {
            throw std::logic_error("Attempted to access the error of a successful Result");
        }
        return std::get<1>(storage_);
    }

    [[nodiscard]] const Error& error() const& {
        if (has_value()) {
            throw std::logic_error("Attempted to access the error of a successful Result");
        }
        return std::get<1>(storage_);
    }

private:
    template <typename... Args>
    explicit Result(std::in_place_index_t<0> tag, Args&&... args)
        : storage_(tag, std::forward<Args>(args)...) {}

    template <typename... Args>
    explicit Result(std::in_place_index_t<1> tag, Args&&... args)
        : storage_(tag, std::forward<Args>(args)...) {}

    std::variant<T, Error> storage_;
};

template <>
class [[nodiscard]] Result<void> final {
public:
    [[nodiscard]] static Result success() {
        return Result{};
    }

    [[nodiscard]] static Result failure(Error error) {
        return Result(std::move(error));
    }

    [[nodiscard]] bool has_value() const noexcept {
        return !error_.has_value();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] Error& error() & {
        if (!error_.has_value()) {
            throw std::logic_error("Attempted to access the error of a successful Result");
        }
        return *error_;
    }

    [[nodiscard]] const Error& error() const& {
        if (!error_.has_value()) {
            throw std::logic_error("Attempted to access the error of a successful Result");
        }
        return *error_;
    }

private:
    Result() = default;
    explicit Result(Error error) : error_(std::move(error)) {}

    std::optional<Error> error_;
};

} // namespace Storm
