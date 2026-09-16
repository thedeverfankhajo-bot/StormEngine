#pragma once

#include <cstdint>

namespace storm::ecs {

enum class SystemPhase : std::uint8_t {
    Input,
    Gameplay,
    Physics,
    Animation,
    RenderPreparation,
    Render,
    Count
};

} // namespace storm::ecs
