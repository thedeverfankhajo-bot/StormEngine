#pragma once

#include "GameLoop.hpp"
#include <string_view>

namespace storm {

class Engine final {
public:
    Engine() = default;
    ~Engine() = default;

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] std::string_view version() const noexcept;

    GameLoop& gameLoop() noexcept { return gameLoop_; }
    const GameLoop& gameLoop() const noexcept { return gameLoop_; }

private:
    GameLoop gameLoop_;
};

} // namespace storm
