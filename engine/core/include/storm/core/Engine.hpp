#pragma once

#include <string_view>

namespace storm {

class Engine final {
public:
    Engine() = default;
    ~Engine() = default;

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] std::string_view version() const noexcept;
};

} // namespace storm
