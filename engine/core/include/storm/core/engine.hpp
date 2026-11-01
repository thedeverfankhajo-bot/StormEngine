#pragma once

#include "storm/foundation/result.hpp"

#include <string>

namespace Storm {

enum class EngineState {
    Created,
    Initialized,
    Shutdown,
};

struct EngineConfig final {
    std::string application_name;
};

// Foundation lifecycle only. Platform startup and the game loop are not implemented yet.
class Engine final {
public:
    Engine() = default;

    [[nodiscard]] Result<void> initialize(EngineConfig config);
    [[nodiscard]] Result<void> shutdown();

    [[nodiscard]] EngineState state() const noexcept;
    [[nodiscard]] const EngineConfig& config() const noexcept;

private:
    EngineState state_{EngineState::Created};
    EngineConfig config_{};
};

} // namespace Storm
