#include "storm/core/Engine.hpp"

namespace storm {

std::string_view Engine::name() const noexcept {
    return "StormEngine";
}

std::string_view Engine::version() const noexcept {
    return "0.1.0-dev";
}

} // namespace storm
