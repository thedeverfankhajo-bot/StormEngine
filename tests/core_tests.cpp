#include <cassert>
#include <string_view>

#include "storm/core/Engine.hpp"

int main() {
    const storm::Engine engine;
    assert(engine.name() == std::string_view{"StormEngine"});
    assert(engine.version() == std::string_view{"0.1.0-dev"});
    return 0;
}
