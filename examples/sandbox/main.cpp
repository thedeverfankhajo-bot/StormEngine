#include <iostream>

#include "storm/core/Engine.hpp"

int main() {
    const storm::Engine engine;
    std::cout << engine.name() << ' ' << engine.version() << '\n';
    return 0;
}
